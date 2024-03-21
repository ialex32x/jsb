#include "jsb_runtime.h"

#if JSB_WITH_DEBUGGER
#include "jsb_debugger.h"
#endif

#include "jsb_module_loader.h"
#include "core/string/string_builder.h"

namespace jsb
{
    struct GlobalInitialize
    {
        std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();

        GlobalInitialize()
        {
#if DEV_ENABLED
            constexpr char args[] = "--expose-gc";
            v8::V8::SetFlagsFromString(args, std::size(args) - 1);
#endif
            v8::V8::InitializePlatform(platform.get());
            v8::V8::Initialize();
        }
    };

    struct JavaScriptRuntimeStore
    {
        std::shared_ptr<JavaScriptRuntime> access(void* p_runtime)
        {
            std::shared_ptr<JavaScriptRuntime> rval;
            lock_.lock();
            if (all_runtimes_.has(p_runtime))
            {
                rval = ((JavaScriptRuntime*) p_runtime)->shared_from_this();
            }
            lock_.unlock();
            return rval;
        }

        void add(void* p_runtime)
        {
            lock_.lock();
            jsb_check(!all_runtimes_.has(p_runtime));
            all_runtimes_.insert(p_runtime);
            lock_.unlock();
        }

        void remove(void* p_runtime)
        {
            lock_.lock();
            jsb_check(all_runtimes_.has(p_runtime));
            all_runtimes_.erase(p_runtime);
            lock_.unlock();
        }

        jsb_force_inline static JavaScriptRuntimeStore& get_shared()
        {
            static JavaScriptRuntimeStore global_store;
            return global_store;
        }

    private:
        BinaryMutex lock_;
        HashSet<void*> all_runtimes_;
    };

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

    namespace
    {
        void PromiseRejectCallback_(v8::PromiseRejectMessage message)
        {
            if (message.GetEvent() != v8::kPromiseRejectWithNoHandler)
            {
                return;
            }

            const v8::Local<v8::Promise> promise = message.GetPromise();
            v8::Isolate* isolate = promise->GetIsolate();
            const v8::Isolate::Scope isolateScope(isolate);
            const v8::HandleScope handleScope(isolate);

            const v8::Local<v8::Value> value = message.GetValue();
            const v8::Local<v8::Context> context = isolate->GetCurrentContext();

            const v8::MaybeLocal<v8::String> maybe = value->ToString(context);
            v8::Local<v8::String> str;
            if (maybe.ToLocal(&str) && str->Length() != 0)
            {
                const v8::String::Utf8Value str_utf8_value(isolate, str);
                const size_t len = str_utf8_value.length();
                const String temp_str(*str_utf8_value, (int) len);

                //TODO get the 'stack' property
                JSB_LOG(Error, "Unhandled promise rejection: %s", temp_str);
            }
        }
    }

    std::shared_ptr<JavaScriptRuntime> JavaScriptRuntime::safe_wrap(void* p_pointer)
    {
        return JavaScriptRuntimeStore::get_shared().access(p_pointer);
    }

    void JavaScriptRuntime::on_context_created(const v8::Local<v8::Context>& p_context)
    {
#if JSB_WITH_DEBUGGER
        debugger_->on_context_created(p_context);
#endif
    }

    void JavaScriptRuntime::on_context_destroyed(const v8::Local<v8::Context>& p_context)
    {
#if JSB_WITH_DEBUGGER
        debugger_->on_context_destroyed(p_context);
#endif
    }

    JavaScriptRuntime::JavaScriptRuntime()
    {
        static GlobalInitialize global_initialize;
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = &allocator_;

        thread_id_ = Thread::get_caller_id();
        isolate_ = v8::Isolate::New(create_params);
        isolate_->SetData(kIsolateEmbedderData, this);
        isolate_->SetPromiseRejectCallback(PromiseRejectCallback_);

        module_loaders_.insert("godot", memnew(GodotModuleLoader));
        JavaScriptRuntimeStore::get_shared().add(this);
#if JSB_WITH_DEBUGGER
        debugger_ = JavaScriptDebugger::create(isolate_);
#endif
    }

    JavaScriptRuntime::~JavaScriptRuntime()
    {
#if JSB_WITH_DEBUGGER
        debugger_.reset();
#endif
        JavaScriptRuntimeStore::get_shared().remove(this);

        timer_manager_.clear_all();

        for (IModuleResolver* resolver : module_resolvers_)
        {
            memdelete(resolver);
        }
        module_resolvers_.clear();

        for (KeyValue<String, IModuleLoader*>& pair : module_loaders_)
        {
            memdelete(pair.value);
            pair.value = nullptr;
        }

        // cleanup weak callbacks not invoked by v8
        while (!objects_.is_empty())
        {
            const internal::Index64 first_index = objects_.get_first_index();
            ObjectHandle& handle = objects_.get_value(first_index);
            const bool is_persistent = persistent_objects_.has(handle.pointer);

            JavaScriptClassInfo& class_info = classes_.get_value(handle.class_id);
            class_info.finalizer(this, handle.pointer, is_persistent);
            handle.ref_.Reset();
            objects_index_.erase(handle.pointer);
            objects_.remove_at(first_index);
            if (is_persistent) persistent_objects_.erase(handle.pointer);
        }

        // cleanup all class templates
        while (!classes_.is_empty())
        {
            const internal::Index32 first_index = classes_.get_first_index();
            JavaScriptClassInfo& class_info = classes_.get_value(first_index);
            class_info.template_.Reset();
            classes_.remove_at(first_index);
        }

        isolate_->Dispose();
        isolate_ = nullptr;
    }

    void JavaScriptRuntime::update()
    {
        const uint64_t base_ticks = Engine::get_singleton()->get_frame_ticks();
        const uint64_t elapsed_milli = (base_ticks - last_ticks_) / 1000; // milliseconds

        last_ticks_ = base_ticks;
        if (timer_manager_.tick(elapsed_milli))
        {
            v8::Isolate::Scope isolate_scope(isolate_);
            v8::HandleScope handle_scope(isolate_);

            timer_manager_.invoke_timers(isolate_);
        }
        isolate_->PerformMicrotaskCheckpoint();
#if JSB_WITH_DEBUGGER
        debugger_->update();
#endif
    }

    void JavaScriptRuntime::gc()
    {
#if DEV_ENABLED
        isolate_->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
#else
        isolate_->LowMemoryNotification();
#endif
    }

    void JavaScriptRuntime::bind_object(internal::Index32 p_class_id, Object* p_pointer, const v8::Local<v8::Object>& p_object, bool p_persistent)
    {
        bind_object(p_class_id, (void*) p_pointer, p_object, p_persistent);
        p_pointer->set_instance_binding(this, p_pointer, gd_instance_binding_callbacks);
    }

    void JavaScriptRuntime::bind_object(internal::Index32 p_class_id, void* p_pointer, const v8::Local<v8::Object>& p_object, bool p_persistent)
    {
        jsb_checkf(classes_.is_valid_index(p_class_id), "bad class_id");
        jsb_checkf(!objects_index_.has(p_pointer), "duplicated bindings");

        const internal::Index64 object_id = objects_.add({});
        ObjectHandle& handle = objects_.get_value(object_id);

        handle.class_id = p_class_id;
        handle.pointer = p_pointer;
        handle.ref_.Reset(isolate_, p_object);
        objects_index_.insert(p_pointer, object_id);
        p_object->SetAlignedPointerInInternalField(kObjectFieldPointer, p_pointer);
        if (p_persistent)
        {
            persistent_objects_.insert(p_pointer);
            handle.ref_count_ = 1;
        }
        else
        {
            handle.ref_.SetWeak(p_pointer, &object_gc_callback, v8::WeakCallbackType::kInternalFields);
        }
        JSB_LOG(Verbose, "bind object %s class_id %d", itos((int64_t) object_id), (int32_t) p_class_id);
    }

    void JavaScriptRuntime::unbind_object(void* p_pointer)
    {
        //TODO thread-safety issues on objects_* access
        jsb_check(Thread::get_caller_id() == thread_id_);
        if (objects_index_.has(p_pointer))
        {
            free_object(p_pointer, false);
        }
    }

    bool JavaScriptRuntime::reference_object(void* p_pointer, bool p_is_inc)
    {
        //TODO temp code
        //TODO thread-safety issues on objects_* access
        jsb_check(Thread::get_caller_id() == thread_id_);

        const HashMap<void*, internal::Index64>::Iterator it = objects_index_.find(p_pointer);
        if (it == objects_index_.end())
        {
            ERR_FAIL_V_MSG(true, "bad pointer");
        }
        const internal::Index64 object_id = it->value;

        ObjectHandle& object_handle = objects_.get_value(object_id);

        // adding references
        if (p_is_inc)
        {
            if (object_handle.ref_count_ == 0)
            {
                // becomes a strong reference
                jsb_check(!object_handle.ref_.IsEmpty());
                object_handle.ref_.ClearWeak();
            }
            ++object_handle.ref_count_;
            return false;
        }

        // removing references
        jsb_checkf(!object_handle.ref_.IsEmpty(), "removing references on dead values");
        if (object_handle.ref_count_ == 0)
        {
            return true;
        }
        // jsb_checkf(object_handle.ref_count_ > 0, "unexpected behaviour");

        --object_handle.ref_count_;
        if (object_handle.ref_count_ == 0)
        {
            object_handle.ref_.SetWeak(p_pointer, &object_gc_callback, v8::WeakCallbackType::kInternalFields);
            return true;
        }
        return false;
    }

    void JavaScriptRuntime::free_object(void* p_pointer, bool p_free)
    {
        jsb_check(Thread::get_caller_id() == thread_id_);
        const HashMap<void*, internal::Index64>::Iterator it = objects_index_.find(p_pointer);
        ERR_FAIL_COND_MSG(it == objects_index_.end(), "bad pointer");
        const internal::Index64 object_id = it->value;

        ObjectHandle& object_handle = objects_.get_value(object_id);
        jsb_check(object_handle.pointer == p_pointer);
        const bool is_persistent = persistent_objects_.has(p_pointer);

        // remove index at first to make `free_object` safely reentrant
        if (is_persistent) persistent_objects_.erase(p_pointer);
        objects_index_.erase(p_pointer);

        if (p_free)
        {
            const JavaScriptClassInfo& class_info = classes_.get_value(object_handle.class_id);
            class_info.finalizer(this, p_pointer, is_persistent);
        }
        object_handle.ref_.Reset();
        // `object_handle` becomes invalid after `remove_at`, so do it finally.
        objects_.remove_at(object_id);
    }

}
