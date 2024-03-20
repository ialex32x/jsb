#ifndef JAVASCRIPT_RUNTIME_H
#define JAVASCRIPT_RUNTIME_H

#include "jsb_pch.h"
#include "jsb_array_buffer_allocator.h"
#include "jsb_object_handle.h"
#include "jsb_class_info.h"
#include "jsb_module_loader.h"
#include "jsb_module_resolver.h"
#include "jsb_timer_action.h"
#include "../internal/jsb_sarray.h"
#include "../internal/jsb_timer_manager.h"

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
        // `p_pointer` should point to a JavaScriptRuntime instance
        jsb_no_discard
        static std::shared_ptr<JavaScriptRuntime> safe_wrap(void* p_pointer);

        jsb_no_discard
        static
        jsb_force_inline
        JavaScriptRuntime* wrap(v8::Isolate* p_isolate)
        {
            return (JavaScriptRuntime*) p_isolate->GetData(kIsolateEmbedderData);
        }

        jsb_no_discard
        jsb_force_inline
        v8::Isolate* unwrap() const { return isolate_; }

        jsb_no_discard
        jsb_force_inline
        bool check(v8::Isolate* p_isolate) const { return p_isolate == isolate_; }

        void bind_object(internal::Index32 p_class_id, void *p_pointer, const v8::Local<v8::Object>& p_object);
        void unbind_object(void* p_pointer);

        jsb_force_inline bool check_object(void* p_pointer) const
        {
            return objects_index_.has(p_pointer);
        }

        jsb_force_inline bool check_object(void* p_pointer, v8::Local<v8::Value>& r_unwrap) const
        {
            const HashMap<void*, internal::Index64>::ConstIterator& it = objects_index_.find(p_pointer);
            if (it != objects_index_.end())
            {
                const ObjectHandle& handle = objects_.get_value(it->value);
                r_unwrap = handle.callback.Get(isolate_);
                return true;
            }
            return false;
        }

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

        class IModuleResolver* find_module_resolver(const String& p_module_id, String& r_asset_path) const
        {
            for (IModuleResolver* resolver : module_resolvers_)
            {
                if (resolver->get_source_info(p_module_id, r_asset_path))
                {
                    return resolver;
                }
            }

            return nullptr;
        }

        template<typename T, typename... ArgumentTypes>
        T& add_module_resolver(ArgumentTypes... p_args)
        {
            T* resolver = memnew(T(p_args...));
            module_resolvers_.append(resolver);
            return *resolver;
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
        void on_context_created(const v8::Local<v8::Context>& p_context);
        void on_context_destroyed(const v8::Local<v8::Context>& p_context);

        static void object_gc_callback(const v8::WeakCallbackInfo<void>& info)
        {
            JavaScriptRuntime* cruntime = wrap(info.GetIsolate());
            cruntime->free_object(info.GetParameter(), true);
        }

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
        Vector<IModuleResolver*> module_resolvers_;

        uint64_t last_ticks_;
        internal::TTimerManager<JavaScriptTimerAction> timer_manager_;

#if JSB_WITH_DEBUGGER
        std::unique_ptr<class JavaScriptDebugger> debugger_;
#endif

        friend class JavaScriptContext;
    };
}

#endif
