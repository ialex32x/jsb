#include "jsb_context.h"

#include "jsb_editor_utility.h"
#include "jsb_exception_info.h"
#include "core/string/string_builder.h"

#include "jsb_module_loader.h"
#include "jsb_transpiler.h"
#include "../internal/jsb_path_util.h"
#include "core/core_constants.h"

namespace jsb
{
    template<typename T>
    jsb_force_inline int32_t jsb_downscale(int64_t p_val, const T& p_msg)
    {
#if DEV_ENABLED
        if (p_val != (int64_t) (int32_t) p_val) JSB_LOG(Warning, "inconsistent int64_t conversion: %s", p_msg);
#endif
        return (int32_t) p_val;
    }

    jsb_force_inline int32_t jsb_downscale(int64_t p_val)
    {
#if DEV_ENABLED
        if (p_val != (int64_t) (int32_t) p_val) JSB_LOG(Warning, "inconsistent int64_t conversion");
#endif
        return (int32_t) p_val;
    }

    namespace InternalTimerType { enum Type : uint8_t { Interval, Timeout, Immediate, }; }

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
        const String combined_id = internal::PathUtil::combine(internal::PathUtil::dirname(p_parent_id), p_module_id);
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
            const CharString cmodule_id = normalized_id.utf8();
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

            // build the module tree
            if (!p_parent_id.is_empty())
            {
                if (const JavaScriptModule* cparent_module = module_cache_.find(p_parent_id))
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

        {
            sym_class_id_.Reset(isolate, v8::Symbol::New(isolate));
        }

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
                editor->Set(context, v8::String::NewFromUtf8Literal(isolate, "get_global_constants"), v8::Function::New(context, JavaScriptEditorUtility::_get_global_constants).ToLocalChecked()).Check();
                editor->Set(context, v8::String::NewFromUtf8Literal(isolate, "get_singletons"), v8::Function::New(context, JavaScriptEditorUtility::_get_singletons).ToLocalChecked()).Check();
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
        case InternalTimerType::Interval: loop = true;
        case InternalTimerType::Timeout:  // NOLINT(clang-diagnostic-implicit-fallthrough)
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
            info.GetReturnValue().Set(v8::Int32::NewFromUnsigned(isolate, (uint32_t) handle));
        }
        else
        {
            cruntime->timer_manager_.set_timer(handle, JavaScriptTimerAction(v8::Global<v8::Function>(isolate, func), 0), rate, loop);
            info.GetReturnValue().Set((uint32_t) handle);
        }
    }

    void JavaScriptContext::_clear_timer(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        if (info.Length() < 1 || !info[0]->IsUint32())
        {
            isolate->ThrowError("bad argument");
            return;
        }

        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        uint32_t handle = 0;
        if (!info[0]->Uint32Value(context).To(&handle))
        {
            isolate->ThrowError("bad timer");
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
        context->SetAlignedPointerInEmbedderData(kContextEmbedderData, nullptr);

        sym_class_id_.Reset();
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

    //TODO stub functions for accessing fields of classes
    jsb_force_inline static real_t Vector2_x_getter(Vector2* self) { return self->x; }
    jsb_force_inline static void Vector2_x_setter(Vector2* self, real_t x) { self->x = x; }
    jsb_force_inline static real_t Vector2_y_getter(Vector2* self) { return self->y; }
    jsb_force_inline static void Vector2_y_setter(Vector2* self, real_t y) { self->y = y; }

    jsb_force_inline static real_t Vector3_x_getter(Vector3* self) { return self->x; }
    jsb_force_inline static void Vector3_x_setter(Vector3* self, real_t x) { self->x = x; }
    jsb_force_inline static real_t Vector3_y_getter(Vector3* self) { return self->y; }
    jsb_force_inline static void Vector3_y_setter(Vector3* self, real_t y) { self->y = y; }
    jsb_force_inline static real_t Vector3_z_getter(Vector3* self) { return self->z; }
    jsb_force_inline static void Vector3_z_setter(Vector3* self, real_t z) { self->z = z; }

    jsb_force_inline static real_t Vector4_x_getter(Vector4* self) { return self->x; }
    jsb_force_inline static void Vector4_x_setter(Vector4* self, real_t x) { self->x = x; }
    jsb_force_inline static real_t Vector4_y_getter(Vector4* self) { return self->y; }
    jsb_force_inline static void Vector4_y_setter(Vector4* self, real_t y) { self->y = y; }
    jsb_force_inline static real_t Vector4_z_getter(Vector4* self) { return self->z; }
    jsb_force_inline static void Vector4_z_setter(Vector4* self, real_t z) { self->z = z; }
    jsb_force_inline static real_t Vector4_w_getter(Vector4* self) { return self->w; }
    jsb_force_inline static void Vector4_w_setter(Vector4* self, real_t w) { self->w = w; }

    //TODO just a temp test, try to read javascript class info from module cache with the given module_id
    //TODO `module_id` is not file path based for now, will it be better to directly use the path as `module_id`??
    Error JavaScriptContext::dump(const String &p_module_id, JavaScriptClassInfo &r_class_info)
    {
        const JavaScriptModule* module = module_cache_.find(p_module_id);
        if (!module)
        {
            return ERR_FILE_NOT_FOUND;
        }

        v8::Isolate* isolate = get_isolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Local<v8::Object> exports = module->exports.Get(isolate);
        v8::Local<v8::Value> default_val;
        if (!exports->Get(context, v8::String::NewFromUtf8Literal(isolate, "default")).ToLocal(&default_val)
            || !default_val->IsObject())
        {
            return ERR_CANT_RESOLVE;
        }

        //TODO
        v8::Local<v8::Object> default_obj = default_val.As<v8::Object>();
        v8::Local<v8::String> name_str = default_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "name")).ToLocalChecked().As<v8::String>();
        v8::String::Utf8Value name(isolate, name_str);
        String cname(*name, name.length());

        r_class_info.name = cname;
        v8::Local<v8::Value> class_id_val;
        const v8::Local<v8::Symbol> class_id_symbol = sym_class_id_.Get(isolate);
        if (!default_obj->Get(context, class_id_symbol).ToLocal(&class_id_val) || !class_id_val->IsUint32())
        {
            // ignore a javascript which does not inherit from native class (directly and indirectly both)
            return ERR_UNAUTHORIZED;
        }

        // unsafe
        const NativeClassInfo& native_class_info = runtime_->get_class((internal::Index32) class_id_val->Uint32Value(context).ToChecked());

        JSB_LOG(Verbose, "class name %s", cname);
        JSB_LOG(Verbose, "native class? %s", native_class_info.name);
        r_class_info.native = native_class_info.name;

        //TODO collect methods/signals/properties
        return OK;
    }

    void JavaScriptContext::expose_temp()
    {
        v8::Isolate* isolate = get_isolate();
        v8::HandleScope handle_scope(isolate);
        v8::Isolate::Scope isolate_scope(isolate);
        v8::Local<v8::Context> context = context_.Get(isolate);
        v8::Local<v8::Object> global = context->Global();

        {
            internal::Index32 class_id;
            const StringName class_name = jsb_typename(Vector2);
            NativeClassInfo& class_info = runtime_->add_class(NativeClassInfo::GodotPrimitive, class_name, &class_id);
            runtime_->godot_primitives_index_[Variant::VECTOR2] = class_id;

            v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector2>::create<real_t, real_t>(isolate, class_id, class_info);
            v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

            // methods
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector2, dot));
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector2, move_toward));
            bind::property(isolate, prototype_template, function_pointers_, Vector2_x_getter, Vector2_x_setter, jsb_nameof(Vector2, x));
            bind::property(isolate, prototype_template, function_pointers_, Vector2_y_getter, Vector2_y_setter, jsb_nameof(Vector2, y));

            // type
            global->Set(context, v8::String::NewFromUtf8Literal(isolate, jsb_typename(Vector2)), function_template->GetFunction(context).ToLocalChecked()).Check();
        }
        {
            internal::Index32 class_id;
            const StringName class_name = jsb_typename(Vector3);
            NativeClassInfo& class_info = runtime_->add_class(NativeClassInfo::GodotPrimitive, class_name, &class_id);
            runtime_->godot_primitives_index_[Variant::VECTOR3] = class_id;

            v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector3>::create<real_t, real_t, real_t>(isolate, class_id, class_info);
            v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

            // methods
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector3, dot));
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector3, move_toward));
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector3, octahedron_encode));
            bind::property(isolate, prototype_template, function_pointers_, Vector3_x_getter, Vector3_x_setter, jsb_nameof(Vector3, x));
            bind::property(isolate, prototype_template, function_pointers_, Vector3_y_getter, Vector3_y_setter, jsb_nameof(Vector3, y));
            bind::property(isolate, prototype_template, function_pointers_, Vector3_z_getter, Vector3_z_setter, jsb_nameof(Vector3, z));

            // type
            global->Set(context, v8::String::NewFromUtf8Literal(isolate, jsb_typename(Vector3)), function_template->GetFunction(context).ToLocalChecked()).Check();
        }
        {
            internal::Index32 class_id;
            const StringName class_name = jsb_typename(Vector4);
            NativeClassInfo& class_info = runtime_->add_class(NativeClassInfo::GodotPrimitive, class_name, &class_id);
            runtime_->godot_primitives_index_[Variant::VECTOR4] = class_id;

            v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector4>::create<real_t, real_t, real_t, real_t>(isolate, class_id, class_info);
            v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

            // methods
            bind::method(isolate, prototype_template, function_pointers_, jsb_methodbind(Vector4, dot));
            bind::property(isolate, prototype_template, function_pointers_, Vector4_x_getter, Vector4_x_setter, jsb_nameof(Vector4, x));
            bind::property(isolate, prototype_template, function_pointers_, Vector4_y_getter, Vector4_y_setter, jsb_nameof(Vector4, y));
            bind::property(isolate, prototype_template, function_pointers_, Vector4_z_getter, Vector4_z_setter, jsb_nameof(Vector4, z));
            bind::property(isolate, prototype_template, function_pointers_, Vector4_w_getter, Vector4_w_setter, jsb_nameof(Vector4, w));

            // type
            global->Set(context, v8::String::NewFromUtf8Literal(isolate, jsb_typename(Vector4)), function_template->GetFunction(context).ToLocalChecked()).Check();
        }
    }

    NativeClassInfo* JavaScriptContext::_expose_godot_variant(internal::Index32* r_class_id)
    {
        //TODO uncertain
        // add_class(None);
        // register(Transpiler<Vector2>::xxx);
        return nullptr;
    }

    NativeClassInfo* JavaScriptContext::_expose_godot_class(const ClassDB::ClassInfo* p_class_info, internal::Index32* r_class_id)
    {
        if (!p_class_info)
        {
            if (r_class_id)
            {
                *r_class_id = internal::Index32::none();
            }
            return nullptr;
        }

        const HashMap<StringName, internal::Index32>::Iterator found = runtime_->godot_classes_index_.find(p_class_info->name);
        if (found != runtime_->godot_classes_index_.end())
        {
            if (r_class_id)
            {
                *r_class_id = found->value;
            }
            return &runtime_->classes_.get_value(found->value);
        }

        internal::Index32 class_id;
        NativeClassInfo& jclass_info = runtime_->add_class(NativeClassInfo::GodotObject, p_class_info->name, &class_id);
        JSB_LOG(Verbose, "expose godot type %s (%d)", p_class_info->name, (uint32_t) class_id);

        // construct type template
        {
            v8::Isolate* isolate = get_isolate();
            v8::Local<v8::Context> context = isolate->GetCurrentContext();

            jsb_check(context == context_.Get(isolate));
            v8::Local<v8::FunctionTemplate> function_template = ClassTemplate<Object>::create(isolate, class_id, jclass_info);
            v8::Local<v8::ObjectTemplate> object_template = function_template->PrototypeTemplate();

            // expose class methods
            for (const KeyValue<StringName, MethodBind*>& pair : p_class_info->method_map)
            {
                const StringName& method_name = pair.key;
                MethodBind* method_bind = pair.value;
                const CharString cmethod_name = ((String) method_name).ascii();
                v8::Local<v8::String> propkey_name = v8::String::NewFromUtf8(isolate, cmethod_name.ptr(), v8::NewStringType::kNormal, cmethod_name.length()).ToLocalChecked();
                v8::Local<v8::FunctionTemplate> propval_func = v8::FunctionTemplate::New(isolate, _godot_object_method, v8::External::New(isolate, method_bind));

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
                const CharString constant_name = ((String) pair.key).utf8(); // utf-8 for better compatibilities
                v8::Local<v8::String> prop_key = v8::String::NewFromUtf8(isolate, constant_name.ptr(), v8::NewStringType::kNormal, constant_name.length()).ToLocalChecked();

                function_template->Set(prop_key, v8::Int32::New(isolate, jsb_downscale(constant_value, pair.key)));
            }

            //TODO expose all available fields/properties etc.

            // set `class_id` on the exposed godot class for the convenience when finding it from any subclasses in javascript.
            // currently used in `dump(in module_id, out class_info)`
            const v8::Local<v8::Symbol> class_id_symbol = sym_class_id_.Get(isolate);
            function_template->Set(class_id_symbol, v8::Uint32::NewFromUnsigned(isolate, class_id));

            // setup the prototype chain (inherit)
            internal::Index32 super_id;
            if (const NativeClassInfo* jsuper_class = _expose_godot_class(p_class_info->inherits_ptr, &super_id))
            {
                v8::Local<v8::FunctionTemplate> base_template = jsuper_class->template_.Get(isolate);
                jsb_check(!base_template.IsEmpty());
                function_template->Inherit(base_template);
                JSB_LOG(Debug, "%s (%d) extends %s (%d)", p_class_info->name, (uint32_t) class_id, p_class_info->inherits_ptr->name, (uint32_t) super_id);
            }
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

        case Variant::OBJECT:
            //TODO ...
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
            {
                //TODO TEMP SOLUTION
                if (p_jval->IsObject())
                {
                    v8::Local<v8::Object> jobj = p_jval.As<v8::Object>();
                    if (jobj->InternalFieldCount() == kObjectFieldCount)
                    {
                        //TODO check the class to make it safe to cast (space cheaper?)
                        //TODO or, add one more InternalField to ensure it (time cheaper?)
                        void* pointer = jobj->GetAlignedPointerFromInternalField(isolate, kObjectFieldPointer);
                        const JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
                        if (const NativeClassInfo* class_info = cruntime->get_object_class(pointer))
                        {
                            if (class_info->type == NativeClassInfo::GodotPrimitive)
                            {
                                r_cvar = *(Variant*) pointer;
                            }
                        }
                    }
                }
                return false;
            }
        default: return false;
        }
    }

    jsb_force_inline bool gd_obj_to_js(v8::Isolate* isolate, const v8::Local<v8::Context>& context, Object* p_cvar, v8::Local<v8::Value>& r_jval, bool is_persistent)
    {
        if (unlikely(!p_cvar))
        {
            r_jval = v8::Null(isolate);
            return true;
        }
        JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
        if (cruntime->check_object(p_cvar, r_jval))
        {
            return true;
        }

        // freshly bind existing gd object (not constructed in javascript)
        const StringName& class_name = p_cvar->get_class_name();
        JavaScriptContext* ccontext = JavaScriptContext::wrap(context);
        internal::Index32 jclass_id;
        if (const NativeClassInfo* jclass = ccontext->_expose_godot_class(class_name, &jclass_id))
        {
            v8::Local<v8::FunctionTemplate> jtemplate = jclass->template_.Get(isolate);
            r_jval = jtemplate->InstanceTemplate()->NewInstance(context).ToLocalChecked();
            jsb_check(r_jval.As<v8::Object>()->InternalFieldCount() == kObjectFieldCount);

            if (p_cvar->is_ref_counted())
            {
                //NOTE in the case this godot object created by a godot method which returns a Ref<T>,
                //     it's `refcount_init` will be zero after the object pointer assigned to a Variant.
                //     we need to resurrect the object from this special state, or it will be a dangling pointer.
                if (((RefCounted*) p_cvar)->init_ref())
                {
                    // great, it's resurrected.
                }
            }

            // the lifecycle will be managed by javascript runtime, DO NOT DELETE it externally
            cruntime->bind_object(jclass_id, p_cvar, r_jval.As<v8::Object>(), is_persistent);
            return true;
        }
        JSB_LOG(Error, "failed to expose godot class '%s'", class_name);
        return false;
    }

    jsb_force_inline bool gd_var_to_js(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const Variant& p_cvar, v8::Local<v8::Value>& r_jval)
    {
        switch (const Variant::Type var_type = p_cvar.get_type())
        {
        case Variant::NIL: r_jval = v8::Null(isolate); return true;
        case Variant::BOOL: r_jval = v8::Boolean::New(isolate, p_cvar); return true;
        case Variant::INT:
            {
                const int64_t raw_val = p_cvar;
                r_jval = v8::Int32::New(isolate, jsb_downscale(raw_val));
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
        case Variant::STRING_NAME:
            {
                //TODO losing type
                const String raw_val2 = p_cvar;
                const CharString repr_val = raw_val2.utf8();
                r_jval = v8::String::NewFromUtf8(isolate, repr_val.get_data(), v8::NewStringType::kNormal, repr_val.length()).ToLocalChecked();
                return true;
            }
        case Variant::OBJECT:
            {
                return gd_obj_to_js(isolate, context, (Object*) p_cvar, r_jval, false);
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
        case Variant::NODE_PATH:
        case Variant::RID:
            //TODO unimplemented
            return false;
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
            {
                //TODO TEMP SOLUTION
                JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
                if (const internal::Index32 class_id = cruntime->get_class_id(var_type))
                {
                    const NativeClassInfo& class_info = cruntime->get_class(class_id);
                    v8::Local<v8::FunctionTemplate> jtemplate = class_info.template_.Get(isolate);
                    r_jval = jtemplate->InstanceTemplate()->NewInstance(context).ToLocalChecked();
                    jsb_check(r_jval.As<v8::Object>()->InternalFieldCount() == kObjectFieldCount);

                    // the lifecycle will be managed by javascript runtime, DO NOT DELETE it externally
                    cruntime->bind_object(class_id, p_cvar, r_jval.As<v8::Object>(), false);
                    return true;
                }
                return false;
            }
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

            // avoid unexpected `this` in a relatively cheap way
            if (!self->IsObject() || self->InternalFieldCount() != kObjectFieldCount)
            {
                isolate->ThrowError("bad this");
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

        //NOTE keep the same order with `GDScriptLanguage::init()`
        // firstly, singletons have the top priority (in GDScriptLanguage::init, singletons will overwrite the globals slot even if a type/const has the same name)
        // checking before getting to avoid error prints
        if (Engine::get_singleton()->has_singleton(type_name))
        if (Object* gd_singleton = Engine::get_singleton()->get_singleton_object(type_name))
        {
            v8::Local<v8::Value> rval;
            JSB_LOG(Verbose, "exposing singleton object %s", (String) type_name);
            if (gd_obj_to_js(isolate, context, gd_singleton, rval, true))
            {
                jsb_check(!rval.IsEmpty());
                info.GetReturnValue().Set(rval);
                return;
            }
            isolate->ThrowError("failed to bind a singleton object");
            return;
        }

        // then, math_defs. but `PI`, `INF`, `NAN`, `TAU` could be easily accessed in javascript.
        // so we just happily omit these defines here.

        // thirdly, global_constants
        if (CoreConstants::is_global_constant(type_name))
        {
            const int constant_index = CoreConstants::get_global_constant_index(type_name);
            const int64_t constant_value = CoreConstants::get_global_constant_value(constant_index);
            const int32_t scaled_value = (int32_t) constant_value;
            if ((int64_t) scaled_value != constant_value)
            {
                JSB_LOG(Warning, "integer overflowed %s (%s) [reversible? %d]", type_name, itos(constant_value), (int64_t)(double) constant_value == constant_value);
                info.GetReturnValue().Set(v8::Number::New(isolate, (double) constant_value));
            }
            else
            {
                info.GetReturnValue().Set(v8::Int32::New(isolate, scaled_value));
            }
            return;
        }

        // finally, classes in ClassDB
        HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(type_name);
        if (it != ClassDB::classes.end())
        {
            if (NativeClassInfo* godot_class = ccontext->_expose_godot_class(&it->value))
            {
                info.GetReturnValue().Set(godot_class->template_.Get(isolate)->GetFunction(context).ToLocalChecked());
                return;
            }
        }

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
        if (const JavaScriptModule* cmain_module = module_cache_.get_main())
        {
            if (r_main_module)
            {
                *r_main_module = cmain_module->module.Get(get_isolate());
            }
            return true;
        }
        return false;
    }

    bool JavaScriptContext::validate(const String &p_source, JavaScriptExceptionInfo *r_err)
    {
        //TODO try to compile?
        return true;
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
