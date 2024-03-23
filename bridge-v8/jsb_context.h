#ifndef JAVASCRIPT_CONTEXT_H
#define JAVASCRIPT_CONTEXT_H

#include "jsb_pch.h"
#include "jsb_runtime.h"
#include "jsb_module.h"

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
    public:
        JavaScriptContext(const std::shared_ptr<class JavaScriptRuntime>& runtime);
        ~JavaScriptContext();

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

        Error dump(const String& p_module_id, JavaScriptClassInfo& r_class_info);

        //TODO temp code
        jsb_deleteme
        void expose_temp();

        // jsb_force_inline uint8_t* get_function_pointer(uint32_t p_offset) { return function_pointers_[p_offset]; }
        jsb_force_inline static uint8_t* get_function(const v8::Local<v8::Context>& p_context, uint32_t p_offset)
        {
            return wrap(p_context)->function_pointers_[p_offset];
        }

        //NOTE AVOID USING THIS CALL, CONSIDERING REMOVING IT.
        //     eval from source
        jsb_deprecated
        Error eval(const CharString& p_source, const String& p_filename);

        // load a script (as module)
        Error load(const String& p_name);

        //TODO is there a simple way to compile (validate) the script without any side effect?
        bool validate(const String& p_name, struct JavaScriptExceptionInfo* r_err = nullptr);

        jsb_force_inline v8::Isolate* get_isolate() const { jsb_check(runtime_); return runtime_->isolate_; }

        //NOTE handle scope is required to call this method
        jsb_force_inline v8::MaybeLocal<v8::Value> _compile_run(const CharString& p_source, const String& p_filename)
        {
            return _compile_run(p_source.ptr(), p_source.length(), p_filename);
        }

        v8::MaybeLocal<v8::Value> _compile_run(const char* p_source, int p_source_len, const String& p_filename);

        bool _get_main_module(v8::Local<v8::Object>* r_main_module) const;

        // JS function (type_name: string): type
        static void _load_type(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _require(const v8::FunctionCallbackInfo<v8::Value>& info);
        NativeClassInfo* _expose_godot_class(const ClassDB::ClassInfo* p_class_info, internal::Index32* r_class_id = nullptr);
        NativeClassInfo* _expose_godot_class(const StringName& p_class_name, internal::Index32* r_class_id = nullptr)
        {
            const HashMap<StringName, ClassDB::ClassInfo>::ConstIterator& it = ClassDB::classes.find(p_class_name);
            return it != ClassDB::classes.end() ? _expose_godot_class(&it->value, r_class_id) : nullptr;
        }
        NativeClassInfo* _expose_godot_variant(internal::Index32* r_class_id);

    private:
        static void _print(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _set_timer(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _clear_timer(const v8::FunctionCallbackInfo<v8::Value>& info);

        static void _godot_object_method(const v8::FunctionCallbackInfo<v8::Value>& info);

        void _register_builtins(const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& self);

        // return false if something wrong with an exception thrown
        // caller should handle the exception if it's not called from js
        bool _load_module(const String& p_parent_id, const String& p_module_id, JavaScriptModule*& r_module);

        // hold a reference to JavaScriptRuntime which ensure runtime being destructed after context
        std::shared_ptr<class JavaScriptRuntime> runtime_;

        internal::FunctionPointers function_pointers_;
        JavaScriptModuleCache module_cache_;
        v8::Global<v8::Object> jmodule_cache_;

        // symbol for class_id on FunctionTemplate of native class
        v8::Global<v8::Symbol> sym_class_id_;

        // main context
        v8::Global<v8::Context> context_;
    };
}

#endif
