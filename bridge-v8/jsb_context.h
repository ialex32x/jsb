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

    struct FunctionPointers
    {
        FunctionPointers(): cursor_(0) { pointer_.resize(16 * 512); }

        template<typename Func>
        uint32_t add(Func func)
        {
            uint32_t last_cursor = cursor_;
            if (pointer_.size() - last_cursor <= sizeof(func))
            {
                pointer_.resize(pointer_.size() * 2);
            }
            memcpy(pointer_.ptrw() + last_cursor, &func, sizeof(func));
            cursor_ += sizeof(func);
            return last_cursor;
        }

        uint8_t* operator[](uint32_t p_index) { return pointer_.ptrw() + p_index; }

        uint32_t cursor_;
        Vector<uint8_t> pointer_;
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

        jsb_force_inline static JavaScriptContext* get(const v8::Local<v8::Context>& p_context)
        {
            return (JavaScriptContext*) p_context->GetAlignedPointerFromEmbedderData(kContextEmbedderData);
        }

        jsb_force_inline bool check(const v8::Local<v8::Context>& p_context) const
        {
            return context_ == p_context;
        }

        //TODO temp code
        void expose();

        // jsb_force_inline uint8_t* get_function_pointer(uint32_t p_offset) { return function_pointers_[p_offset]; }
        jsb_force_inline static uint8_t* get_function(const v8::Local<v8::Context>& p_context, uint32_t p_offset)
        {
            return get(p_context)->function_pointers_[p_offset];
        }

        Error eval(const CharString& p_source, const CharString& p_filename);

        // load a script (as module)
        Error load(const String& p_name);

        jsb_force_inline v8::Isolate* get_isolate() const { jsb_check(runtime_); return runtime_->isolate_; }

        //NOTE handle scope is required to call this method
        jsb_force_inline v8::MaybeLocal<v8::Value> _compile_run(const CharString& p_source, const CharString& p_filename)
        {
            return _compile_run(p_source.ptr(), p_source.length(), p_filename);
        }

        v8::MaybeLocal<v8::Value> _compile_run(const char* p_source, int p_source_len, const CharString& p_filename);

        bool _get_main_module(v8::Local<v8::Object>* r_main_module) const;

        // JS function (type_name: string): type
        static void _load_type(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _require(const v8::FunctionCallbackInfo<v8::Value>& info);

    private:
        struct ExceptionInfo
        {
            String message;
            // String stack;
        };

        static void _print(const v8::FunctionCallbackInfo<v8::Value>& info);

        static void _godot_object_constructor(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _godot_object_finalizer(void* pointer);
        static void _godot_object_method(const v8::FunctionCallbackInfo<v8::Value>& info);

        void _register_builtins(const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& self);
        JavaScriptClassInfo* _expose_godot_class(const ClassDB::ClassInfo* p_class_info, internal::Index32* r_class_id = nullptr);

        // return false if something wrong with an exception thrown
        // caller should handle the exception if it's not called from js
        bool _load_module(const String& p_parent_id, const String& p_module_id, JavaScriptModule*& r_module);

        bool _read_exception(const v8::TryCatch& p_catch, ExceptionInfo& r_info);

        // hold a reference to JavaScriptRuntime which ensure runtime being destructed after context
        std::shared_ptr<class JavaScriptRuntime> runtime_;

        FunctionPointers function_pointers_;
        JavaScriptModuleCache module_cache_;
        v8::Global<v8::Object> jmodule_cache_;

        // main context
        v8::Global<v8::Context> context_;
    };
}

#endif
