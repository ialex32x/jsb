#include "jsb_context.h"

#include "jsb_editor_utility.h"
#include "jsb_exception_info.h"
#include "core/string/string_builder.h"

#include "jsb_module_loader.h"
#include "jsb_transpiler.h"
#include "../internal/jsb_path_util.h"

namespace jsb
{
    namespace InternalTimerType { enum Type : uint8_t { Interval, Timeout, Immediate, }; }

    struct InstanceBindingCallbacks
    {
        jsb_force_inline operator const GDExtensionInstanceBindingCallbacks* () const { return &callbacks_; }

        InstanceBindingCallbacks()
            : callbacks_ { create_callback, free_callback, reference_callback }
        {}

    private:
        static void* create_callback(void* p_token, void* p_instance)
        {
            //TODO ??
            JSB_LOG(Error, "unimplemented");
            return nullptr;
        }

        static void free_callback(void* p_token, void* p_instance, void* p_binding)
        {
            if (std::shared_ptr<JavaScriptRuntime> cruntime = JavaScriptRuntime::safe_wrap(p_token))
            {
                jsb_check(p_instance == p_binding);
                cruntime->unbind_object(p_binding);
            }
        }

        static GDExtensionBool reference_callback(void* p_token, void* p_binding, GDExtensionBool p_reference)
        {
            if (std::shared_ptr<JavaScriptRuntime> cruntime = JavaScriptRuntime::safe_wrap(p_token))
            {
                return cruntime->reference_object(p_binding, !!p_reference);
            }
            return true;
        }

        GDExtensionInstanceBindingCallbacks callbacks_;
    };

    namespace { InstanceBindingCallbacks gd_instance_binding_callbacks = {}; }

    void JavaScriptContext::_print(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        v8::Local<v8::Int32> magic;
        if (!info.Data()->ToInt32(context).ToLocal(&magic))
        {
            isolate->ThrowError("bad call");
            return;
        }

        StringBuilder sb;
        sb.append("[jsb]");
        const internal::ELogSeverity::Type severity = (internal::ELogSeverity::Type) magic->Value();
        int index = severity != internal::ELogSeverity::Assert ? 0 : 1;
        if (index == 1)
        {
            // check assertion
            if (info[0]->BooleanValue(isolate))
            {
                return;
            }
        }

        // join all data
        for (int n = info.Length(); index < n; index++)
        {
            v8::String::Utf8Value str(isolate, info[index]);

            if (str.length() > 0)
            {
                sb.append(" ");
                sb.append(String::utf8(*str, str.length()));
            }
        }

        switch (severity)
        {
        case internal::ELogSeverity::Assert: CRASH_NOW_MSG(sb.as_string()); return;
        case internal::ELogSeverity::Error: ERR_FAIL_MSG(sb.as_string()); return;
        case internal::ELogSeverity::Warning: WARN_PRINT(sb.as_string()); return;
        case internal::ELogSeverity::Trace: //TODO append stacktrace
        default: print_line(sb.as_string()); return;
        }
    }

    void JavaScriptContext::_require(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        if (info.Length() != 1)
        {
            isolate->ThrowError("one argument required");
            return;
        }

        v8::Local<v8::Value> arg0 = info[0];
        if (!arg0->IsString())
        {
            isolate->ThrowError("bad argument");
            return;
        }

        // read parent module id from magic data
        String parent_id = JavaScriptModuleCache::get_name(isolate, info.Data());
        String module_id = JavaScriptModuleCache::get_name(isolate, arg0);

        JavaScriptContext* ccontext = JavaScriptContext::wrap(context);
        JavaScriptModule* module;
        if (ccontext->_load_module(parent_id, module_id, module))
        {
            info.GetReturnValue().Set(module->exports);
        }
    }

    bool JavaScriptContext::_load_module(const String& p_parent_id, const String& p_module_id, JavaScriptModule*& r_module)
    {
        if (JavaScriptModule* module = module_cache_.find(p_module_id))
        {
            r_module = module;
            return true;
        }

        v8::Isolate* isolate = runtime_->isolate_;
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        jsb_check(context == context_.Get(isolate));
        // find loader with the module id
        if (IModuleLoader* loader = runtime_->find_module_loader(p_module_id))
        {
            JavaScriptModule& module = module_cache_.insert(p_module_id, false);
            v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
            v8::Local<v8::String> propkey_loaded = v8::String::NewFromUtf8Literal(isolate, "loaded");

            // register the new module obj into module_cache obj
            v8::Local<v8::Object> jmodule_cache = jmodule_cache_.Get(isolate);
            CharString cmodule_id = p_module_id.utf8();
            v8::Local<v8::String> jmodule_id = v8::String::NewFromUtf8(isolate, cmodule_id.ptr(), v8::NewStringType::kNormal, cmodule_id.length()).ToLocalChecked();
            jmodule_cache->Set(context, jmodule_id, module_obj).Check();

            // init the new module obj
            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, false)).Check();
            module.id = p_module_id;
            module.module.Reset(isolate, module_obj);

            //NOTE the loader should throw error if failed
            if (!loader->load(this, module))
            {
                return false;
            }

            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, true)).Check();
            r_module = &module;
            return true;
        }

        // try resolve the module id
        String combined_id = internal::PathUtil::combine(internal::PathUtil::dirname(p_parent_id), p_module_id);
        String normalized_id;
        if (internal::PathUtil::extract(combined_id, normalized_id) != OK || normalized_id.is_empty())
        {
            isolate->ThrowError("bad path");
            return false;
        }

        // init source module
        String asset_path;
        if (IModuleResolver* resolver = runtime_->find_module_resolver(normalized_id, asset_path))
        {
            // supported module properties: id, filename, cache, loaded, exports, children
            JavaScriptModule& module = module_cache_.insert(normalized_id, true);
            CharString cmodule_id = normalized_id.utf8();
            v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
            v8::Local<v8::String> propkey_loaded = v8::String::NewFromUtf8Literal(isolate, "loaded");
            v8::Local<v8::String> propkey_children = v8::String::NewFromUtf8Literal(isolate, "children");

            // register the new module obj into module_cache obj
            v8::Local<v8::Object> jmodule_cache = jmodule_cache_.Get(isolate);
            v8::Local<v8::String> jmodule_id = v8::String::NewFromUtf8(isolate, cmodule_id.ptr(), v8::NewStringType::kNormal, cmodule_id.length()).ToLocalChecked();
            jmodule_cache->Set(context, jmodule_id, module_obj).Check();

            // init the new module obj
            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, false)).Check();
            module_obj->Set(context, v8::String::NewFromUtf8Literal(isolate, "id"), jmodule_id).Check();
            module_obj->Set(context, v8::String::NewFromUtf8Literal(isolate, "cache"), jmodule_cache).Check();
            module_obj->Set(context, propkey_children, v8::Array::New(isolate)).Check();
            module.id = normalized_id;
            module.module.Reset(isolate, module_obj);

            //NOTE the resolver should throw error if failed
            //NOTE module.filename should be set in `resolve.load`
            if (!resolver->load(this, asset_path, module))
            {
                return false;
            }

            if (!p_parent_id.is_empty())
            {
                if (JavaScriptModule* cparent_module = module_cache_.find(p_parent_id))
                {
                    v8::Local<v8::Object> jparent_module = cparent_module->module.Get(isolate);
                    v8::Local<v8::Value> jparent_children_v;
                    if (jparent_module->Get(context, propkey_children).ToLocal(&jparent_children_v) && jparent_children_v->IsArray())
                    {
                        v8::Local<v8::Array> jparent_children = jparent_children_v.As<v8::Array>();
                        const uint32_t children_num = jparent_children->Length();
                        jparent_children->Set(context, children_num, module_obj).Check();
                    }
                    else
                    {
                        JSB_LOG(Error, "can not access children on '%s'", p_parent_id);
                    }
                    module_obj->Set(context, v8::String::NewFromUtf8Literal(isolate, "parent"), jparent_module).Check();
                }
                else
                {
                    JSB_LOG(Warning, "parent module not found with the name '%s'", p_parent_id);
                }
            }
            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, true)).Check();
            r_module = &module;
            return true;
        }

        const CharString cerror_message = vformat("unknown module: %s", normalized_id).utf8();
        isolate->ThrowError(v8::String::NewFromUtf8(isolate, cerror_message.ptr(), v8::NewStringType::kNormal, cerror_message.length()).ToLocalChecked());
        return false;
    }

    void JavaScriptContext::_register_builtins(const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& self)
    {
        v8::Isolate* isolate = runtime_->isolate_;

        // internal 'jsb'
        {
            v8::Local<v8::Object> jhost = v8::Object::New(isolate);

            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "jsb"), jhost).Check();
            jhost->Set(context, v8::String::NewFromUtf8Literal(isolate, "DEV_ENABLED"), v8::Boolean::New(isolate, DEV_ENABLED)).Check();
            jhost->Set(context, v8::String::NewFromUtf8Literal(isolate, "TOOLS_ENABLED"), v8::Boolean::New(isolate, TOOLS_ENABLED)).Check();

#if TOOLS_ENABLED
            // internal 'jsb.editor'
            {
                v8::Local<v8::Object> editor = v8::Object::New(isolate);

                jhost->Set(context, v8::String::NewFromUtf8Literal(isolate, "editor"), editor).Check();
                editor->Set(context, v8::String::NewFromUtf8Literal(isolate, "get_classes"), v8::Function::New(context, JavaScriptEditorUtility::_get_classes).ToLocalChecked()).Check();
            }
#endif
        }

        // minimal console functions support
        {
            v8::Local<v8::Object> jconsole = v8::Object::New(isolate);

            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "console"), jconsole).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "log"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Log)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "info"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Info)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "debug"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Debug)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "warn"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Warning)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "error"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Error)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "assert"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Assert)).ToLocalChecked()).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "trace"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Trace)).ToLocalChecked()).Check();
        }

        // the root 'require' function
        {
            v8::Local<v8::Object> jmodule_cache = v8::Object::New(isolate);
            v8::Local<v8::Function> require_func = v8::Function::New(context, _require).ToLocalChecked();
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "require"), require_func).Check();
            require_func->Set(context, v8::String::NewFromUtf8Literal(isolate, "cache"), jmodule_cache).Check();
            require_func->Set(context, v8::String::NewFromUtf8Literal(isolate, "moduleId"), v8::String::Empty(isolate)).Check();
            jmodule_cache_.Reset(isolate, jmodule_cache);
        }

        //TODO the root 'import' function (async module loading?)
        {

        }

        // essential timer support
        {
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "setInterval"), v8::Function::New(context, _set_timer, v8::Int32::New(isolate, InternalTimerType::Interval)).ToLocalChecked()).Check();
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "setTimeout"), v8::Function::New(context, _set_timer, v8::Int32::New(isolate, InternalTimerType::Timeout)).ToLocalChecked()).Check();
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "setImmediate"), v8::Function::New(context, _set_timer, v8::Int32::New(isolate, InternalTimerType::Immediate)).ToLocalChecked()).Check();
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "clearInterval"), v8::Function::New(context, _clear_timer).ToLocalChecked()).Check();
        }
    }

    void JavaScriptContext::_set_timer(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        const int argc = info.Length();
        if (argc < 1 || !info[0]->IsFunction())
        {
            isolate->ThrowError("bad argument");
            return;
        }

        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        int32_t type;
        if (!info.Data()->Int32Value(context).To(&type))
        {
            isolate->ThrowError("bad call");
            return;
        }
        int32_t rate = 1;
        int extra_arg_index = 1;
        bool loop = false;
        switch ((InternalTimerType::Type) type)
        {
        // interval & timeout have 2 arguments usually
        case InternalTimerType::Interval: loop = true; // intentionally omit the break;
        case InternalTimerType::Timeout:
            if (!info[1]->IsUndefined() && !info[1]->Int32Value(context).To(&rate))
            {
                isolate->ThrowError("bad time");
                return;
            }
            extra_arg_index = 2;
            break;
        // immediate has 1 argument usually
        default: break;
        }

        JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
        v8::Local<v8::Function> func = info[0].As<v8::Function>();
        internal::TimerHandle handle;

        if (argc > extra_arg_index)
        {
            JavaScriptTimerAction action(v8::Global<v8::Function>(isolate, func), argc - extra_arg_index);
            for (int index = extra_arg_index; index < argc; ++index)
            {
                action.store(index - extra_arg_index, v8::Global<v8::Value>(isolate, info[index]));
            }
            cruntime->timer_manager_.set_timer(handle, std::move(action), rate, loop);
            info.GetReturnValue().Set(v8::Int32::New(isolate, (int32_t) handle));
        }
        else
        {
            cruntime->timer_manager_.set_timer(handle, JavaScriptTimerAction(v8::Global<v8::Function>(isolate, func), 0), rate, loop);
            info.GetReturnValue().Set(v8::Int32::New(isolate, (int32_t) handle));
        }
    }

    void JavaScriptContext::_clear_timer(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        if (info.Length() < 1 || !info[0]->IsInt32())
        {
            isolate->ThrowError("bad argument");
            return;
        }

        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        int32_t handle = 0;
        if (!info[0]->Int32Value(context).To(&handle))
        {
            isolate->ThrowError("bad time");
            return;
        }
        JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
        cruntime->timer_manager_.clear_timer((internal::TimerHandle) (internal::Index32) handle);
    }

    JavaScriptContext::JavaScriptContext(const std::shared_ptr<JavaScriptRuntime>& runtime)
        : runtime_(runtime)
    {
        v8::Isolate* isolate = runtime->isolate_;
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);

        v8::Local<v8::Context> context = v8::Context::New(isolate);
        context->SetAlignedPointerInEmbedderData(kContextEmbedderData, this);
        context_.Reset(isolate, context);
        {
            v8::Context::Scope context_scope(context);
            v8::Local<v8::Object> global = context->Global();

            _register_builtins(context, global);
        }
        runtime_->on_context_created(context);
    }

    JavaScriptContext::~JavaScriptContext()
    {
        v8::Isolate* isolate = runtime_->isolate_;
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(get_isolate());

        runtime_->on_context_destroyed(context);
        {
            context->SetAlignedPointerInEmbedderData(kContextEmbedderData, nullptr);
        }

        jmodule_cache_.Reset();
        context_.Reset();
    }

    Error JavaScriptContext::load(const String& p_name)
    {
        v8::Isolate* isolate = get_isolate();
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Context::Scope context_scope(context);

        JavaScriptModule* module;
        v8::TryCatch try_catch_run(isolate);
        if (_load_module("", p_name, module))
        {
            // no exception should thrown if module loaded successfully
            jsb_check(!try_catch_run.HasCaught());
            return OK;
        }

        if (JavaScriptExceptionInfo exception_info = JavaScriptExceptionInfo(isolate, try_catch_run))
        {
            ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, (String) exception_info);
        }
        ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, "something wrong");
    }

    struct Foo
    {
        Foo()
        {
            print_line("[cpp] Foo constructor");
        }

        ~Foo()
        {
            print_line("[cpp] Foo destructor");
        }

        int test(int i)
        {
            print_line(vformat("[jsb] Foo.test %d", i));
            return i + 1;
        }
    };

    void JavaScriptContext::expose()
    {
        v8::Isolate* isolate = get_isolate();
        v8::HandleScope handle_scope(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Local<v8::Object> global = context->Global();

        internal::Index32 class_id;
        JavaScriptClassInfo& class_info = runtime_->add_class(&class_id);
        class_info.constructor = &Transpiler<Foo>::constructor;
        class_info.finalizer = &Transpiler<Foo>::finalizer;

        v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate,
            class_info.constructor, v8::Uint32::NewFromUnsigned(isolate, class_id));
        function_template->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);
        //class_info.template_.Reset(isolate, function_template);

        // methods
        function_template->PrototypeTemplate()->Set(
            v8::String::NewFromUtf8Literal(isolate, "test"),
            v8::FunctionTemplate::New(isolate,
                &Transpiler<Foo>::dispatch<int, int>,
                v8::Uint32::NewFromUnsigned(isolate, function_pointers_.add(&Foo::test))));

        //function_template->Inherit()

        // type
        global->Set(context,
            v8::String::NewFromUtf8Literal(isolate, "Foo"),
            function_template->GetFunction(context).ToLocalChecked()).Check();
    }

    void JavaScriptContext::_godot_object_finalizer(void* pointer)
    {
        Object* gd_object = (Object*) pointer;
        if (gd_object->is_ref_counted())
        {
            if (((RefCounted*) gd_object)->unreference())
            {
                memdelete(gd_object);
            }
        }
        else
        {
            //TODO only delete when the object's lifecycle is fully managed by javascript
            memdelete(gd_object);
        }
    }

    void JavaScriptContext::_godot_object_constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::Local<v8::Object> self = info.This();
        v8::Local<v8::Uint32> data = v8::Local<v8::Uint32>::Cast(info.Data());
        internal::Index32 class_id(data->Value());

        JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
        JavaScriptClassInfo& jclass_info = cruntime->classes_.get_value(class_id);
        HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(jclass_info.name);
        jsb_check(it != ClassDB::classes.end());
        ClassDB::ClassInfo& gd_class_info = it->value;

        Object* gd_object = gd_class_info.creation_func();
        // self->SetAlignedPointerInInternalField(kObjectFieldPointer, gd_object);
        cruntime->bind_object(class_id, gd_object, self);
        if (gd_object->is_ref_counted())
        {
            //NOTE IS IT A TRUTH that ref_count==1 after creation_func??
            jsb_check(!((RefCounted*) gd_object)->is_referenced());
        }

        gd_object->set_instance_binding(cruntime, gd_object, gd_instance_binding_callbacks);
    }

    JavaScriptClassInfo* JavaScriptContext::_expose_godot_class(const ClassDB::ClassInfo* p_class_info, internal::Index32* r_class_id)
    {
        if (!p_class_info)
        {
            if (r_class_id)
            {
                *r_class_id = internal::Index32::none();
            }
            return nullptr;
        }

        HashMap<StringName, internal::Index32>::Iterator found = runtime_->godot_classes_index_.find(p_class_info->name);
        if (found != runtime_->godot_classes_index_.end())
        {
            if (r_class_id)
            {
                *r_class_id = found->value;
            }
            return &runtime_->classes_.get_value(found->value);
        }

        internal::Index32 class_id;
        JavaScriptClassInfo& jclass_info = runtime_->add_class(&class_id);
        runtime_->godot_classes_index_.insert(p_class_info->name, class_id);
        JSB_LOG(Verbose, "load godot type %s (%d)", p_class_info->name, (uint32_t) class_id);

        //TODO code for constructing type template
        {
            v8::Isolate* isolate = get_isolate();
            v8::Local<v8::Context> context = isolate->GetCurrentContext();

            jsb_check(context == context_.Get(isolate));
            jclass_info.name = p_class_info->name;
            jclass_info.constructor = &_godot_object_constructor;
            jclass_info.finalizer = &_godot_object_finalizer;
            v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, jclass_info.constructor, v8::Uint32::NewFromUnsigned(isolate, class_id));

            v8::Local<v8::ObjectTemplate> object_template = function_template->PrototypeTemplate();
            function_template->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);
#if DEV_ENABLED
            const CharString cname = String(p_class_info->name).utf8();
            function_template->SetClassName(v8::String:: NewFromUtf8(isolate, cname.ptr(), v8::NewStringType::kNormal, cname.length()).ToLocalChecked());
#endif

            // expose class methods
            for (const KeyValue<StringName, MethodBind*>& pair : p_class_info->method_map)
            {
                const StringName& method_name = pair.key;
                MethodBind* method_bind = pair.value;
                const CharString cmethod_name = ((String) method_name).ascii();

                v8::Local<v8::String> propkey_name = v8::String::NewFromUtf8(isolate, cmethod_name.ptr(), v8::NewStringType::kNormal, cmethod_name.length()).ToLocalChecked();
                v8::Local<v8::FunctionTemplate> propval_func = v8::FunctionTemplate::New(isolate, _godot_object_method, v8::External::New(isolate, method_bind));

                JSB_LOG(Verbose, "expose %s.%s", p_class_info->name, method_name);
                if (method_bind->is_static())
                {
                    function_template->Set(propkey_name, propval_func);
                }
                else
                {
                    object_template->Set(propkey_name, propval_func);
                }
            }

            // expose class constants
            for (const KeyValue<StringName, int64_t>& pair : p_class_info->constant_map)
            {
                const int64_t constant_value = pair.value;
                const int32_t scaled_value = (int32_t) constant_value;
                const CharString constant_name = ((String) pair.key).utf8(); // utf-8 for better compatibilities

                v8::Local<v8::String> prop_key = v8::String::NewFromUtf8(isolate, constant_name.ptr(), v8::NewStringType::kNormal, constant_name.length()).ToLocalChecked();

                if ((int64_t) scaled_value != constant_value)
                {
                    JSB_LOG(Warning, "inconsistent binding %s %d", pair.key, constant_value);
                }
                function_template->Set(prop_key, v8::Int32::New(isolate, scaled_value));
            }

            //TODO expose all available fields/properties etc.

            jclass_info.template_.Reset(isolate, function_template);
        }

        // setup the prototype chain
        internal::Index32 super_id;
        if (JavaScriptClassInfo* jsuper_class = _expose_godot_class(p_class_info->inherits_ptr, &super_id))
        {
            v8::Isolate* isolate = get_isolate();

            jclass_info.super_ = super_id;
            v8::Local<v8::FunctionTemplate> this_template = jclass_info.template_.Get(isolate);
            v8::Local<v8::FunctionTemplate> base_template = jsuper_class->template_.Get(isolate);
            this_template->Inherit(base_template);
            JSB_LOG(Debug, "%s (%d) extends %s (%d)", p_class_info->name, (uint32_t) class_id, p_class_info->inherits_ptr->name, (uint32_t) super_id);
        }

        if (r_class_id)
        {
            *r_class_id = class_id;
        }
        return &jclass_info;
    }

    jsb_force_inline bool js_to_gd_var(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_jval, Variant::Type p_type, Variant& r_cvar)
    {
        switch (p_type)
        {
        case Variant::NIL: return p_jval->IsNullOrUndefined();
        case Variant::BOOL:
            // strict?
            if (p_jval->IsBoolean()) { r_cvar = p_jval->BooleanValue(isolate); return true; }
            return false;
        case Variant::INT:
            // strict?
            if (p_jval->IsInt32()) { r_cvar = p_jval->Int32Value(context).ToChecked(); return true; }
            if (p_jval->IsNumber()) { r_cvar = (int64_t) p_jval->NumberValue(context).ToChecked(); return true; }
            return false;
        case Variant::FLOAT:
            if (p_jval->IsNumber()) { r_cvar = p_jval->NumberValue(context).ToChecked(); return true; }
            return false;
        case Variant::STRING:
            if (p_jval->IsString())
            {
                //TODO optimize with cache?
                v8::String::Utf8Value str(isolate, p_jval);
                r_cvar = String(*str, str.length());
                return true;
            }
            return false;

        // math types
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
        case Variant::TRANSFORM2D:
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
        case Variant::PLANE:
        case Variant::QUATERNION:
        case Variant::AABB:
        case Variant::BASIS:
        case Variant::TRANSFORM3D:
        case Variant::PROJECTION:

        // misc types
        case Variant::COLOR:
        case Variant::STRING_NAME:
        case Variant::NODE_PATH:
        case Variant::RID:
        case Variant::OBJECT:
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::DICTIONARY:
        case Variant::ARRAY:

        // typed arrays
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
        case Variant::PACKED_STRING_ARRAY:
        case Variant::PACKED_VECTOR2_ARRAY:
        case Variant::PACKED_VECTOR3_ARRAY:
        case Variant::PACKED_COLOR_ARRAY:
            //TODO unimplemented
        default: return false;
        }
    }

    jsb_force_inline bool gd_var_to_js(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const Variant& p_cvar, v8::Local<v8::Value>& r_jval)
    {
        switch (p_cvar.get_type())
        {
        case Variant::NIL: r_jval = v8::Null(isolate); return true;
        case Variant::BOOL:
            r_jval = v8::Boolean::New(isolate, p_cvar);
            return true;
        case Variant::INT:
            {
                const int64_t raw_val = p_cvar;
                const int32_t trunc_val = (int32_t) raw_val;
#if DEV_ENABLED
                if (raw_val != (int64_t) trunc_val)
                {
                    JSB_LOG(Warning, "conversion lose precision");
                }
#endif
                r_jval = v8::Int32::New(isolate, trunc_val);
                return true;
            }
        case Variant::FLOAT:
            {
                r_jval = v8::Number::New(isolate, p_cvar);
                return true;
            }
        case Variant::STRING:
            {
                //TODO optimize with cache?
                const String raw_val = p_cvar;
                const CharString repr_val = raw_val.utf8();
                r_jval = v8::String::NewFromUtf8(isolate, repr_val.get_data(), v8::NewStringType::kNormal, repr_val.length()).ToLocalChecked();
                return true;
            }
        // math types
        case Variant::VECTOR2:
        case Variant::VECTOR2I:
        case Variant::RECT2:
        case Variant::RECT2I:
        case Variant::VECTOR3:
        case Variant::VECTOR3I:
        case Variant::TRANSFORM2D:
        case Variant::VECTOR4:
        case Variant::VECTOR4I:
        case Variant::PLANE:
        case Variant::QUATERNION:
        case Variant::AABB:
        case Variant::BASIS:
        case Variant::TRANSFORM3D:
        case Variant::PROJECTION:

        // misc types
        case Variant::COLOR:
        case Variant::STRING_NAME:
            {
                //TODO losing type
                const String raw_val2 = p_cvar;
                const CharString repr_val = raw_val2.utf8();
                r_jval = v8::String::NewFromUtf8(isolate, repr_val.get_data(), v8::NewStringType::kNormal, repr_val.length()).ToLocalChecked();
                return true;
            }
        case Variant::NODE_PATH:
        case Variant::RID:
            //TODO unimplemented
            return false;
        case Variant::OBJECT:
            {
                Object* raw_val = p_cvar;
                if (unlikely(!raw_val))
                {
                    r_jval = v8::Null(isolate);
                    return true;
                }
                JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
                if (cruntime->check_object(raw_val, r_jval))
                {
                    return true;
                }

                // freshly bind existing gd object (not constructed in javascript)
                const StringName& class_name = raw_val->get_class_name();
                JavaScriptContext* ccontext = JavaScriptContext::wrap(context);
                internal::Index32 jclass_id;
                if (JavaScriptClassInfo* jclass = ccontext->_expose_godot_class(class_name, &jclass_id))
                {
                    v8::Local<v8::FunctionTemplate> jtemplate = jclass->template_.Get(isolate);
                    v8::Local<v8::Object> jinst = jtemplate->InstanceTemplate()->NewInstance(context).ToLocalChecked();

                    if (raw_val->is_ref_counted())
                    {
                        //NOTE in the case this godot object created by a godot method which returns a Ref<T>,
                        //     it's `refcount_init` will be zero after the object pointer assigned to a Variant.
                        //     we need to resurrect the object from this special state, or it will be a dangling pointer.
                        if (((RefCounted*) raw_val)->init_ref())
                        {
                            // great, it's resurrected.
                        }
                    }
                    // the lifecycle will be managed by javascript runtime, DO NOT DELETE it externally
                    cruntime->bind_object(jclass_id, raw_val, jinst);
                    r_jval = jinst;
                    return true;
                }
                JSB_LOG(Error, "failed to expose godot class '%s'", class_name);
                return false;
            }
        case Variant::CALLABLE:
        case Variant::SIGNAL:
        case Variant::DICTIONARY:
        case Variant::ARRAY:

        // typed arrays
        case Variant::PACKED_BYTE_ARRAY:
        case Variant::PACKED_INT32_ARRAY:
        case Variant::PACKED_INT64_ARRAY:
        case Variant::PACKED_FLOAT32_ARRAY:
        case Variant::PACKED_FLOAT64_ARRAY:
        case Variant::PACKED_STRING_ARRAY:
        case Variant::PACKED_VECTOR2_ARRAY:
        case Variant::PACKED_VECTOR3_ARRAY:
        case Variant::PACKED_COLOR_ARRAY:
            //TODO unimplemented
        default: return false;
        }
    }

    struct GodotArguments
    {
    private:
        int argc_;
        Variant *args_;

    public:
        jsb_force_inline GodotArguments() : argc_(0), args_(nullptr) {}

        jsb_force_inline bool translate(const MethodBind& p_method_bind, v8::Isolate* p_isolate, const v8::Local<v8::Context>& p_context, const v8::FunctionCallbackInfo<v8::Value>& p_info, int& r_failed_arg_index)
        {
            argc_ = p_info.Length();
            args_ = argc_ == 0 ? nullptr : memnew_arr(Variant, argc_);

            //TODO handle vararg call
            if (p_method_bind.is_vararg())
            {

            }

            for (int index = 0; index < argc_; ++index)
            {
                Variant::Type type = p_method_bind.get_argument_type(index);
                if (!js_to_gd_var(p_isolate, p_context, p_info[index], type, args_[index]))
                {
                    r_failed_arg_index = index;
                    return false;
                }
            }

            //TODO use default argument var if not given in js call info
            // p_method_bind.get_default_argument(index);
            return true;
        }

        jsb_force_inline ~GodotArguments()
        {
            if (args_)
            {
                memdelete_arr(args_);
            }
        }

        jsb_force_inline int argc() const { return argc_; }

        jsb_force_inline Variant& operator[](int p_index) { return args_[p_index]; }

        jsb_force_inline void fill(const Variant** p_argv) const
        {
            for (int i = 0; i < argc_; ++i)
            {
                p_argv[i] = &args_[i];
            }
        }

    };

    void JavaScriptContext::_godot_object_method(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        if (!info.Data()->IsExternal())
        {
            isolate->ThrowError("bad call");
            return;
        }

        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Local<v8::External> data = info.Data().As<v8::External>();
        MethodBind* method_bind = (MethodBind*) data->Value();

        jsb_check(method_bind);
        Callable::CallError error;

        Object* gd_object = nullptr;
        if (!method_bind->is_static())
        {
            v8::Local<v8::Object> self = info.This();
            if (self->IsNullOrUndefined())
            {
                isolate->ThrowError("call method without a valid instance bound");
                return;
            }

            // `this` must be a gd object which already bound to javascript
            void* pointer = self->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            jsb_check(JavaScriptRuntime::wrap(isolate)->check_object(pointer));
            gd_object = (Object*) pointer;
        }

        GodotArguments arguments;
        int failed_arg_index;
        if (!arguments.translate(*method_bind, isolate, context, info, failed_arg_index))
        {
            const CharString raw_string = vformat("bad argument: %d", failed_arg_index).ascii();
            v8::Local<v8::String> error_message = v8::String::NewFromOneByte(isolate, (const uint8_t*) raw_string.ptr(), v8::NewStringType::kNormal, raw_string.length()).ToLocalChecked();
            isolate->ThrowError(error_message);
            return;
        }

        //NOTE (unsafe) DO NOT FORGET TO free argv (if it's not stack allocated)
        const int argc = arguments.argc();
        const Variant** argv = (const Variant**)jsb_stackalloc(argc * sizeof(Variant*));

        arguments.fill(argv);
        Variant crval = method_bind->call(gd_object, argv, argc, error);
        jsb_stackfree(argv);

        if (error.error != Callable::CallError::CALL_OK)
        {
            isolate->ThrowError("failed to call");
            return;
        }
        v8::Local<v8::Value> jrval;
        if (gd_var_to_js(isolate, context, crval, jrval))
        {
            info.GetReturnValue().Set(jrval);
            return;
        }
        isolate->ThrowError("failed to translate godot variant to v8 value");
    }

    // [JS] function load_type(type_name: string): Class;
    void JavaScriptContext::_load_type(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::Local<v8::Value> arg0 = info[0];
        if (!arg0->IsString())
        {
            isolate->ThrowError("bad parameter");
            return;
        }

        v8::String::Utf8Value str_utf8(isolate, arg0);
        StringName type_name(*str_utf8);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        JavaScriptContext* ccontext = JavaScriptContext::wrap(context);

        HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(type_name);
        if (it != ClassDB::classes.end())
        {
            if (JavaScriptClassInfo* godot_class = ccontext->_expose_godot_class(&it->value))
            {
                info.GetReturnValue().Set(godot_class->template_.Get(isolate)->GetFunction(context).ToLocalChecked());
                return;
            }
        }

        //TODO singletons? put singletons into another module?

        const CharString message = vformat("godot class not found '%s'", type_name).utf8();
        isolate->ThrowError(v8::String::NewFromUtf8(isolate, message.ptr(), v8::NewStringType::kNormal, message.length()).ToLocalChecked());
    }

    Error JavaScriptContext::eval(const CharString& p_source, const String& p_filename)
    {
        v8::Isolate* isolate = get_isolate();
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Context::Scope context_scope(context);

        v8::TryCatch try_catch_run(isolate);
        /* return value discarded */ _compile_run(p_source, p_filename);
        if (try_catch_run.HasCaught())
        {
            v8::Local<v8::Message> message = try_catch_run.Message();
            v8::Local<v8::Value> stack_trace;
            if (try_catch_run.StackTrace(context).ToLocal(&stack_trace))
            {
                v8::String::Utf8Value stack_trace_utf8(isolate, stack_trace);
                if (stack_trace_utf8.length() != 0)
                {
                    ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, String(*stack_trace_utf8, stack_trace_utf8.length()));
                }
            }

            // fallback to plain message
            const v8::String::Utf8Value message_utf8(isolate, message->Get());
            ERR_FAIL_V_MSG(ERR_COMPILATION_FAILED, String::utf8(*message_utf8, message_utf8.length()));
        }
        return OK;
    }

    bool JavaScriptContext::_get_main_module(v8::Local<v8::Object>* r_main_module) const
    {
        if (JavaScriptModule* cmain_module = module_cache_.get_main())
        {
            if (r_main_module)
            {
                *r_main_module = cmain_module->module.Get(get_isolate());
            }
            return true;
        }
        return false;
    }

    v8::MaybeLocal<v8::Value> JavaScriptContext::_compile_run(const char* p_source, int p_source_len, const String& p_filename)
    {
        v8::Isolate* isolate = get_isolate();
        v8::Local<v8::Context> context = context_.Get(isolate);

#if WINDOWS_ENABLED
        const CharString filename = p_filename.replace("/", "\\").utf8();
#else
        const CharString filename = p_filename.utf8();
#endif
        v8::ScriptOrigin origin(isolate, v8::String::NewFromUtf8(isolate, filename, v8::NewStringType::kNormal, filename.length()).ToLocalChecked());
        v8::MaybeLocal<v8::String> source = v8::String::NewFromUtf8(isolate, p_source, v8::NewStringType::kNormal, p_source_len);
        v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source.ToLocalChecked(), &origin);

        if (script.IsEmpty())
        {
            return {};
        }

        v8::MaybeLocal<v8::Value> maybe_value = script.ToLocalChecked()->Run(context);
        if (maybe_value.IsEmpty())
        {
            return {};
        }

        JSB_LOG(Verbose, "script compiled %s", p_filename);
        return maybe_value;
    }

}
