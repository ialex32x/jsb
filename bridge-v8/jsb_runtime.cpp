#include "jsb_runtime.h"

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

    struct Runtimes
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

    private:
        BinaryMutex lock_;
        HashSet<void*> all_runtimes_;
    };

    static Runtimes runtimes_;

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

    std::shared_ptr<JavaScriptRuntime> JavaScriptRuntime::unwrap(void* p_pointer)
    {
        return runtimes_.access(p_pointer);
    }

    JavaScriptRuntime::JavaScriptRuntime()
    {
        static GlobalInitialize global_initialize;
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator = &allocator_;

        thread_id_ = Thread::get_caller_id();
        pending_request_calls_ = false;
        isolate_ = v8::Isolate::New(create_params);
        isolate_->SetData(kIsolateEmbedderData, this);
        isolate_->SetPromiseRejectCallback(PromiseRejectCallback_);

        module_loaders_.insert("godot", memnew(GodotModuleLoader));
        runtimes_.add(this);
    }

    JavaScriptRuntime::~JavaScriptRuntime()
    {
        runtimes_.remove(this);

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

            JavaScriptClassInfo& class_info = classes_.get_value(handle.class_id);
            class_info.finalizer(handle.pointer);
            handle.callback.Reset();
            handle.ref_.Reset();
            objects_index_.erase(handle.pointer);
            objects_.remove_at(first_index);
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
        // debug_server_->update();
        isolate_->PerformMicrotaskCheckpoint();
    }

    void JavaScriptRuntime::gc()
    {
#if DEV_ENABLED
        isolate_->RequestGarbageCollectionForTesting(v8::Isolate::kFullGarbageCollection);
#else
        isolate_->LowMemoryNotification();
#endif
    }

    void JavaScriptRuntime::object_gc_callback(const v8::WeakCallbackInfo<void>& info)
    {
        JavaScriptRuntime* cruntime = get(info.GetIsolate());
        cruntime->free_object(info.GetParameter(), true);
    }

    void JavaScriptRuntime::bind_object(internal::Index32 p_class_id, void *p_pointer, const v8::Local<v8::Object>& p_object)
    {
        jsb_check(classes_.is_valid_index(p_class_id));
        internal::Index64 object_id = objects_.add({});
        ObjectHandle& handle = objects_.get_value(object_id);

        handle.class_id = p_class_id;
        handle.pointer = p_pointer;
        handle.callback.Reset(isolate_, p_object);
        handle.callback.SetWeak(p_pointer, &object_gc_callback, v8::WeakCallbackType::kInternalFields);
        objects_index_.insert(p_pointer, object_id);
    }

    void JavaScriptRuntime::unbind_object(void* p_pointer)
    {
        //TODO temp code
        //TODO thread-safety issues on objects_* access
        jsb_check(Thread::get_caller_id() == thread_id_);

        free_object(p_pointer, false);
    }

    bool JavaScriptRuntime::reference_object(void* p_pointer, bool p_is_inc)
    {
        //TODO temp code
        //TODO thread-safety issues on objects_* access
        jsb_check(Thread::get_caller_id() == thread_id_);

        HashMap<void*, internal::Index64>::Iterator it = objects_index_.find(p_pointer);
        ERR_FAIL_COND_V_MSG(it == objects_index_.end(), true, "bad pointer");
        const internal::Index64 object_id = it->value;

        ObjectHandle& object_handle = objects_.get_value(object_id);
        if (p_is_inc)
        {
            if (object_handle.ref_count_ == 0)
            {
                //TODO scope issues
                v8::Local<v8::Value> temp = object_handle.callback.Get(isolate_);
                jsb_check(!temp.IsEmpty());
                object_handle.ref_.Reset(isolate_, temp);
            }
            ++object_handle.ref_count_;
            return false;
        }
        if (object_handle.ref_count_ == 0)
        {
            jsb_check(object_handle.ref_.IsEmpty());
            return true;
        }
        --object_handle.ref_count_;
        if (object_handle.ref_count_ == 0)
        {
            object_handle.ref_.Reset();
            return true;
        }
        return false;
    }

    bool JavaScriptRuntime::free_object(void* p_pointer, bool p_free)
    {
        HashMap<void*, internal::Index64>::Iterator it = objects_index_.find(p_pointer);
        ERR_FAIL_COND_V_MSG(it == objects_index_.end(), false, "bad pointer");
        const internal::Index64 object_id = it->value;

        ObjectHandle& object_handle = objects_.get_value(object_id);
        if (p_free)
        {
            JavaScriptClassInfo& class_info = classes_.get_value(object_handle.class_id);
            jsb_check(object_handle.pointer == p_pointer);
            class_info.finalizer(object_handle.pointer);
        }
        object_handle.callback.Reset();
        object_handle.ref_.Reset();
        objects_index_.erase(object_handle.pointer);
        objects_.remove_at(object_id);
        return true;
    }

}
