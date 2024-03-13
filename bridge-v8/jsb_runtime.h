#ifndef JAVASCRIPT_RUNTIME_H
#define JAVASCRIPT_RUNTIME_H

#include "jsb_pch.h"
#include "jsb_array_buffer_allocator.h"
#include "jsb_object_handle.h"
#include "jsb_class_info.h"
#include "jsb_module_loader.h"
#include "jsb_module_resolver.h"
#include "../internal/jsb_sarray.h"

namespace jsb
{
    enum : uint32_t
    {
        kIsolateEmbedderData = 0,
    };

    // JavaScriptRuntime it-self is NOT thread-safe.
    class JavaScriptRuntime : public std::enable_shared_from_this<JavaScriptRuntime>
    {
    public:
        JavaScriptRuntime();
        ~JavaScriptRuntime();

        // return a JavaScriptRuntime pointer via `p_pointer` if it's still alive
        static std::shared_ptr<JavaScriptRuntime> unwrap(void* p_pointer);

        jsb_force_inline bool check(v8::Isolate* p_isolate) const { return p_isolate == isolate_; }

        jsb_force_inline static JavaScriptRuntime* get(v8::Isolate* p_isolate)
        {
            return (JavaScriptRuntime*) p_isolate->GetData(kIsolateEmbedderData);
        }

        void bind_object(internal::Index32 p_class_id, void *p_pointer, const v8::Local<v8::Object>& p_object);
        void unbind_object(void* p_pointer);

        // return true if can die
        bool reference_object(void* p_pointer, bool p_is_inc);

        // request a garbage collection
        void gc();

        void update();

        class IModuleLoader* find_module_loader(const String& p_module_id) const
        {
            HashMap<String, class IModuleLoader*>::ConstIterator it = module_loaders_.find(p_module_id);
            if (it != module_loaders_.end())
            {
                return it->value;
            }
            return nullptr;
        }

        JavaScriptClassInfo& add_class(internal::Index32* o_class_id = nullptr)
        {
            const internal::Index32 class_id = classes_.append(JavaScriptClassInfo());
            JavaScriptClassInfo& class_info = classes_.get_value(class_id);
            // class_info.class_id = class_id;
            if (o_class_id)
            {
                *o_class_id = class_id;
            }
            return class_info;
        }

    private:
        static void object_gc_callback(const v8::WeakCallbackInfo<void>& info);
        bool free_object(void* p_pointer, bool p_free);

        /*volatile*/
        Thread::ID thread_id_;

        v8::Isolate* isolate_;
        ArrayBufferAllocator allocator_;

        //TODO postpone the call of Global.Reset if calling from other threads
        typedef void(*UnreferencingRequestCall)(internal::Index64);
        Vector<UnreferencingRequestCall> request_calls_;
        volatile bool pending_request_calls_;

        //TODO lock on 'objects_' when referencing?
        // lock;

        // indirect lookup
        HashMap<StringName, internal::Index32> godot_classes_index_;

        internal::SArray<JavaScriptClassInfo, internal::Index32> classes_;

        // cpp objects should be added here since the gc callback is not guaranteed by v8
        // we need to delete them on finally releasing JavaScriptRuntime
        internal::SArray<ObjectHandle> objects_;

        // (unsafe) mapping object pointer to object_id
        HashMap<void*, internal::Index64> objects_index_;

        // module_id => loader
        HashMap<String, class IModuleLoader*> module_loaders_;
        SourceModuleResolver source_module_resolver_;

        friend class JavaScriptContext;
    };
}

#endif
