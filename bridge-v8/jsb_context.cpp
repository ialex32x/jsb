#include "jsb_context.h"
#include "core/string/string_builder.h"

#include "jsb_module_loader.h"
#include "jsb_transpiler.h"
#include "../internal/jsb_path_util.h"

namespace jsb
{
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
            if (std::shared_ptr<JavaScriptRuntime> cruntime = JavaScriptRuntime::unwrap(p_token))
            {
                jsb_check(p_instance == p_binding);
                cruntime->unbind_object(p_binding);
            }
        }

        static GDExtensionBool reference_callback(void* p_token, void* p_binding, GDExtensionBool p_reference)
        {
            if (std::shared_ptr<JavaScriptRuntime> cruntime = JavaScriptRuntime::unwrap(p_token))
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
                sb.append(*str);
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

    //TODO require chain (access parent_id in child module's require)
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

        JavaScriptContext* ccontext = JavaScriptContext::get(context);

        // return directly if the module already cached
        if (JavaScriptModule* module = ccontext->module_cache_.find(module_id))
        {
            info.GetReturnValue().Set(module->exports);
            return;
        }

        JavaScriptRuntime* cruntime = JavaScriptRuntime::get(isolate);

        // find loader with the module id
        if (IModuleLoader* loader = cruntime->find_module_loader(module_id))
        {
            JavaScriptModule& module = ccontext->module_cache_.insert(module_id);
            v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
            v8::Local<v8::String> propkey_loaded = v8::String::NewFromUtf8Literal(isolate, "loaded");

            // register the new module obj into module_cache obj
            v8::Local<v8::Object> jmodule_cache = ccontext->jmodule_cache_.Get(isolate);
            jmodule_cache->Set(context, arg0, module_obj).Check();

            // init the new module obj
            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, false));
            module.id = module_id;
            module.module.Reset(isolate, module_obj);

            //NOTE the loader should throw error if failed
            if (loader->load(ccontext, module))
            {
                module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, true));
                info.GetReturnValue().Set(module.exports.Get(isolate));
            }
            return;
        }

        // try resolve the module id
        String combined_id = internal::PathUtil::combine(internal::PathUtil::dirname(parent_id), module_id);
        String normalized_id;
        if (internal::PathUtil::extract(combined_id, normalized_id) != OK || normalized_id.is_empty())
        {
            isolate->ThrowError("bad path");
            return;
        }

        // init source module
        SourceModuleResolver& resolver = cruntime->source_module_resolver_;
        if (resolver.is_valid(normalized_id))
        {
            JavaScriptModule& module = ccontext->module_cache_.insert(normalized_id);
            v8::Local<v8::Object> module_obj = v8::Object::New(isolate);
            v8::Local<v8::String> propkey_loaded = v8::String::NewFromUtf8Literal(isolate, "loaded");

            // register the new module obj into module_cache obj
            v8::Local<v8::Object> jmodule_cache = ccontext->jmodule_cache_.Get(isolate);
            jmodule_cache->Set(context, arg0, module_obj).Check();

            // init the new module obj
            module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, false));
            module.id = normalized_id;
            module.module.Reset(isolate, module_obj);

            //NOTE the resolver should throw error if failed
            if (resolver.load(ccontext, module))
            {
                //TODO
                module_obj->Set(context, propkey_loaded, v8::Boolean::New(isolate, true));
                info.GetReturnValue().Set(module.exports.Get(isolate));
            }
            return;
        }

        isolate->ThrowError("unknown module");
    }

    void JavaScriptContext::_register_builtins(const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& self)
    {
        v8::Isolate* isolate = runtime_->isolate_;

        // minimal console functions support
        {
            v8::Local<v8::Object> jconsole = v8::Object::New(isolate);

            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "console"), jconsole).Check();
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "log"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Verbose)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "info"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Info)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "debug"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Debug)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "warn"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Warning)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "error"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Error)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "assert"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Assert)).ToLocalChecked());
            jconsole->Set(context, v8::String::NewFromUtf8Literal(isolate, "trace"), v8::Function::New(context, _print, v8::Int32::New(isolate, internal::ELogSeverity::Trace)).ToLocalChecked());
        }

        // the root 'require' function
        {
            v8::Local<v8::Object> jmodule_cache = v8::Object::New(isolate);
            v8::Local<v8::Function> require_func = v8::Function::New(context, _require).ToLocalChecked();
            self->Set(context, v8::String::NewFromUtf8Literal(isolate, "require"), require_func).Check();
            require_func->Set(context, v8::String::NewFromUtf8Literal(isolate, "cache"), jmodule_cache);
            require_func->Set(context, v8::String::NewFromUtf8Literal(isolate, "moduleId"), v8::String::Empty(isolate));
            jmodule_cache_.Reset(isolate, jmodule_cache);
        }
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
    }

    JavaScriptContext::~JavaScriptContext()
    {
        jmodule_cache_.Reset();
        context_.Reset();
    }

    void JavaScriptContext::load(const String& p_name)
    {}

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
            function_template->GetFunction(context).ToLocalChecked());
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
    }

    void JavaScriptContext::_godot_object_constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::Local<v8::Object> self = info.This();
        v8::Local<v8::Uint32> data = v8::Local<v8::Uint32>::Cast(info.Data());
        internal::Index32 class_id(data->Value());

        JavaScriptRuntime* cruntime = JavaScriptRuntime::get(isolate);
        JavaScriptClassInfo& jclass_info = cruntime->classes_.get_value(class_id);
        HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(jclass_info.name);
        jsb_check(it != ClassDB::classes.end());
        ClassDB::ClassInfo& gd_class_info = it->value;

        Object* gd_object = gd_class_info.creation_func();
        self->SetAlignedPointerInInternalField(kObjectFieldPointer, gd_object);
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

            jclass_info.name = p_class_info->name;
            jclass_info.constructor = &_godot_object_constructor;
            jclass_info.finalizer = &_godot_object_finalizer;
            v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(isolate, jclass_info.constructor, v8::Uint32::NewFromUnsigned(isolate, class_id));

            function_template->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);
#if DEV_ENABLED
            const CharString cname = String(p_class_info->name).utf8();
            function_template->SetClassName(v8::String:: NewFromUtf8(isolate, cname.ptr(), v8::NewStringType::kNormal, cname.length()).ToLocalChecked());
#endif

            //TODO expose all available fields/methods/functions etc.

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
        JavaScriptContext* ccontext = JavaScriptContext::get(context);

        HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(type_name);
        if (it != ClassDB::classes.end())
        {
            if (JavaScriptClassInfo* godot_class = ccontext->_expose_godot_class(&it->value))
            {
                info.GetReturnValue().Set(godot_class->template_.Get(isolate)->GetFunction(context).ToLocalChecked());
                return;
            }
        }

        const CharString message = vformat("godot class not found '%s'", type_name).utf8();
        isolate->ThrowError(v8::String::NewFromUtf8(isolate, message.ptr(), v8::NewStringType::kNormal, message.length()).ToLocalChecked());
    }

    Error JavaScriptContext::eval(const CharString& p_source, const CharString& p_filename)
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

    v8::MaybeLocal<v8::Value> JavaScriptContext::_compile_run(const char* p_source, int p_source_len, const CharString& p_filename)
    {
        v8::Isolate* isolate = get_isolate();
        v8::Local<v8::Context> context = context_.Get(isolate);

        v8::ScriptOrigin origin(isolate, v8::String::NewFromUtf8(isolate, p_filename, v8::NewStringType::kNormal, p_filename.length()).ToLocalChecked());
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

        return maybe_value;
    }

}
