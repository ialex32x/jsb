#ifndef JAVASCRIPT_CONTEXT_H
#define JAVASCRIPT_CONTEXT_H

#include "jsb_function.h"
#include "jsb_pch.h"
#include "jsb_runtime.h"
#include "jsb_module.h"
#include "jsb_primitive_bindings.h"

namespace jsb
{
    enum : uint32_t
    {
        kContextEmbedderData = 0,
    };

    /**
     * a sandbox-like execution context for scripting (NOT thread-safe)
     * \note members starting with '_' are assumed the a v8 scope already existed in the caller
     */
    class JavaScriptContext //: public std::enable_shared_from_this<JavaScriptContext>
    {
    private:
        friend class JavaScriptLanguage;

        // hold a reference to JavaScriptRuntime which ensure runtime being destructed after context
        std::shared_ptr<class JavaScriptRuntime> runtime_;
        v8::Global<v8::Context> context_;

        internal::FunctionPointers function_pointers_;

        JavaScriptModuleCache module_cache_;
        v8::Global<v8::Object> jmodule_cache_;

        // cache all js functions accessed by GodotJSScript (GodotJSScriptInstance)
        internal::SArray<JavaScriptFunction> js_functions_;

        HashMap<StringName, PrimitiveTypeRegisterFunc> primitive_regs_;

    public:
        JavaScriptContext(const std::shared_ptr<class JavaScriptRuntime>& runtime);
        ~JavaScriptContext();

        void register_primitive_binding(const StringName& p_name, PrimitiveTypeRegisterFunc p_func);

        jsb_force_inline static JavaScriptContext* wrap(const v8::Local<v8::Context>& p_context)
        {
            return (JavaScriptContext*) p_context->GetAlignedPointerFromEmbedderData(kContextEmbedderData);
        }

        jsb_force_inline v8::Local<v8::Context> unwrap() const
        {
            return context_.Get(runtime_->isolate_);
        }

        jsb_force_inline bool check(const v8::Local<v8::Context>& p_context) const
        {
            return context_ == p_context;
        }

        //TODO temp
        jsb_force_inline const GodotJSClassInfo& get_gdjs_class_info(GodotJSClassID p_class_id) const { return runtime_->get_gdjs_class(p_class_id); }

        jsb_force_inline static uint8_t* get_function_pointer(const v8::Local<v8::Context>& p_context, uint32_t p_offset)
        {
            return wrap(p_context)->function_pointers_[p_offset];
        }

        //TODO temp
        GodotJSFunctionID get_function(NativeObjectID p_object_id, const StringName& p_method);
        bool remove_function(GodotJSFunctionID p_func_id)
        {
            return js_functions_.remove_at(p_func_id);
        }
        Variant call_function(NativeObjectID p_object_id, GodotJSFunctionID p_func_id, const Variant **p_args, int p_argcount, Callable::CallError &r_error);

        jsb_force_inline const JavaScriptModuleCache& get_module_cache() const { return module_cache_; }

        //NOTE AVOID USING THIS CALL, CONSIDERING REMOVING IT.
        //     eval from source
        Error eval_source(const CharString& p_source, const String& p_filename);

        /**
         * \brief load a module script
         * \param p_name module_id
         * \return OK if compiled and run with no error
         */
        Error load(const String& p_name);

        //TODO is there a simple way to compile (validate) the script without any side effect?
        bool validate_script(const String& p_path, struct JavaScriptExceptionInfo* r_err = nullptr);

        NativeObjectID crossbind(Object* p_this, GodotJSClassID p_class_id);

        jsb_force_inline v8::Isolate* get_isolate() const { jsb_check(runtime_); return runtime_->isolate_; }

        /**
         * \brief run  and return a value from source
         * \param p_source source bytes (utf-8 encoded)
         * \param p_source_len length of source
         * \param p_filename SourceOrigin (compile the code snippet without ScriptOrigin if `p_filename` is empty)
         * \return js rval
         */
        v8::MaybeLocal<v8::Value> _compile_run(const char* p_source, int p_source_len, const String& p_filename);
        v8::Local<v8::Function> _new_require_func(const CharString& p_module_id)
        {
            v8::Isolate* isolate = runtime_->isolate_;
            v8::Local<v8::Context> context = context_.Get(isolate);
            v8::Local<v8::String> jmodule_id = v8::String::NewFromUtf8(isolate, p_module_id.ptr(), v8::NewStringType::kNormal, p_module_id.length()).ToLocalChecked();
            v8::Local<v8::Function> jrequire = v8::Function::New(context, JavaScriptContext::_require, /* magic: module_id */ jmodule_id).ToLocalChecked();
            v8::Local<v8::Object> jmain_module;
            if (_get_main_module(&jmain_module))
            {
                jrequire->Set(context, v8::String::NewFromUtf8Literal(isolate, "main"), jmain_module).Check();
            }
            else
            {
                JSB_LOG(Warning, "invalid main module");
                jrequire->Set(context, v8::String::NewFromUtf8Literal(isolate, "main"), v8::Undefined(isolate)).Check();
            }
            return jrequire;
        }

        bool _get_main_module(v8::Local<v8::Object>* r_main_module) const;

        // JS function (type_name: string): type
        // it's called from JS, load godot type with the `type_name` in the `godot` module (it can be type/singleton/constant/etc.)
        static void _load_godot_mod(const v8::FunctionCallbackInfo<v8::Value>& info);

        static void _require(const v8::FunctionCallbackInfo<v8::Value>& info);
        NativeClassInfo* _expose_godot_class(const ClassDB::ClassInfo* p_class_info, NativeClassID* r_class_id = nullptr);
        NativeClassInfo* _expose_godot_class(const StringName& p_class_name, NativeClassID* r_class_id = nullptr)
        {
            const HashMap<StringName, ClassDB::ClassInfo>::ConstIterator& it = ClassDB::classes.find(p_class_name);
            return it != ClassDB::classes.end() ? _expose_godot_class(&it->value, r_class_id) : nullptr;
        }

        // return false if something wrong with an exception thrown
        // caller should handle the exception if it's not called from js
        JavaScriptModule* _load_module(const String& p_parent_id, const String& p_module_id);
        void _reload_module(const String& p_module_id);

    private:
        static void _new_callable(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _define(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _print(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _set_timer(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _clear_timer(const v8::FunctionCallbackInfo<v8::Value>& info);

        static void _godot_object_method(const v8::FunctionCallbackInfo<v8::Value>& info);

        void _register_builtins(const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& self);

        void _parse_script_class(v8::Isolate* p_isolate, const v8::Local<v8::Context>& p_context, JavaScriptModule& p_module);
        void _parse_script_class_iterate(v8::Isolate* p_isolate, const v8::Local<v8::Context>& p_context, GodotJSClassInfo& p_class_info);
    };
}

#endif
