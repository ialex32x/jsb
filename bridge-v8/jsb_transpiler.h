#ifndef JAVASCRIPT_TEMPLATE_H
#define JAVASCRIPT_TEMPLATE_H

#include "jsb_pch.h"
#include "jsb_runtime.h"
#include "jsb_context.h"
#include "jsb_object_handle.h"

#if DEV_ENABLED
#   define JSB_CLASS_BOILERPLATE_NAME(TemplateName, ClassName) \
    if ((const void *) ClassName) \
    {\
        const CharString cname = String(ClassName).utf8();\
        TemplateName->SetClassName(v8::String:: NewFromUtf8(isolate, cname.ptr(), v8::NewStringType::kNormal, cname.length()).ToLocalChecked());\
    } (void)0
#else
#   define JSB_CLASS_BOILERPLATE_NAME(TemplateName, ClassName) (void) 0
#endif

#define JSB_CLASS_BOILERPLATE() \
    jsb_force_inline static v8::Local<v8::FunctionTemplate> create(v8::Isolate* isolate, internal::Index32 class_id, JavaScriptClassInfo& class_info)\
    {\
        v8::Local<v8::FunctionTemplate> template_ =  v8::FunctionTemplate::New(isolate, &constructor, v8::Uint32::NewFromUnsigned(isolate, class_id));\
        template_->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);\
        JSB_CLASS_BOILERPLATE_NAME(template_, class_info.name);\
        return template_;\
    }

namespace jsb
{
    template<typename> struct PrimitiveAccess {};

    template<> struct PrimitiveAccess<Vector3>
    {
        static Vector3* from(Variant* p_var) { return VariantInternal::get_vector3(p_var); }
        static Vector3 from(const v8::Local<v8::Value>& p_val)
        {
            return *from((Variant*) p_val.As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer));
        }
    };

    template<> struct PrimitiveAccess<const Vector3&> : PrimitiveAccess<Vector3>
    {
    };

    template<> struct PrimitiveAccess<real_t>
    {
        static bool return_(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::FunctionCallbackInfo<v8::Value>& info, real_t val) { info.GetReturnValue().Set(val); return true; }
    };

    // call with return
    template<typename TReturn>
    struct SpecializedReturn
    {
        template<typename TSelf>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (TSelf::*Functor)();

            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            Functor& func = *(Functor*) JavaScriptContext::get_function(context, info.Data()->Uint32Value(context).ToChecked());
            v8::Local<v8::Object> self = info.This();
            void* pointer = self->GetAlignedPointerFromInternalField(kObjectFieldPointer);

            TSelf* p_self = PrimitiveAccess<TSelf>::from((Variant*) pointer);
            TReturn result = (p_self->*func)();
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        template<typename TSelf, typename P1>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (TSelf::*Functor)(P1);

            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            Functor& func = *(Functor*) JavaScriptContext::get_function(context, info.Data()->Uint32Value(context).ToChecked());
            v8::Local<v8::Object> self = info.This();
            void* pointer = self->GetAlignedPointerFromInternalField(kObjectFieldPointer);

            TSelf* p_self = PrimitiveAccess<TSelf>::from((Variant*) pointer);
            P1 p1 = PrimitiveAccess<P1>::from(info[0]);
            TReturn result = (p_self->*func)(p1);
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        static void function(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (*Functor)();

            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            Functor& func = *(Functor*) JavaScriptContext::get_function(context, info.Data()->Uint32Value(context).ToChecked());

            TReturn result = (*func)();
            //TODO push result
        }
    };

    // call without return
    template<>
    struct SpecializedReturn<void>
    {

    };

    template<typename TSelf>
    struct ClassTemplate
    {
        JSB_CLASS_BOILERPLATE()

        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());

            TSelf* ptr = memnew(TSelf);
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        static void finalizer(JavaScriptRuntime* runtime, void* pointer, bool p_persistent)
        {
            TSelf* self = (TSelf*) pointer;
            if (!p_persistent)
            {
                JSB_LOG(Verbose, "deleting object %d", (uintptr_t) self);
                memdelete(self);
            }
        }
    };

    template<>
    struct ClassTemplate<Object>
    {
        JSB_CLASS_BOILERPLATE()

        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Object> self = info.This();
            const internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());

            JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
            const JavaScriptClassInfo& jclass_info = cruntime->get_class(class_id);
            jsb_check(jclass_info.type == JavaScriptClassType::GodotObject);
            const HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(jclass_info.name);
            jsb_check(it != ClassDB::classes.end());
            const ClassDB::ClassInfo& gd_class_info = it->value;

            Object* gd_object = gd_class_info.creation_func();
            //NOTE IS IT A TRUTH that ref_count==1 after creation_func??
            jsb_check(!gd_object->is_ref_counted() || !((RefCounted*) gd_object)->is_referenced());
            cruntime->bind_object(class_id, gd_object, self, false);
        }

        static void finalizer(JavaScriptRuntime* runtime, void* pointer, bool p_persistent)
        {
            Object* self = (Object*) pointer;
            if (self->is_ref_counted())
            {
                // if we do not `free_instance_binding`, an error will be report on `reference_object`.
                // self->free_instance_binding(runtime);

                if (((RefCounted*) self)->unreference())
                {
                    if (!p_persistent)
                    {
                        JSB_LOG(Verbose, "deleting gd ref_counted object %d", (uintptr_t) self);
                        memdelete(self);
                    }
                }
            }
            else
            {
                //TODO only delete when the object's lifecycle is fully managed by javascript
                if (!p_persistent)
                {
                    JSB_LOG(Verbose, "deleting gd object %d", (uintptr_t) self);
                    memdelete(self);
                }
            }
        }
    };

    namespace bind
    {
        template<typename TSelf, typename TReturn, size_t N>
        static void with(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate>& prototype, internal::FunctionPointers& fp, TReturn (TSelf::*func)(), const char (&name)[N])
        {
            prototype->Set(v8::String::NewFromUtf8Literal(isolate, name), v8::FunctionTemplate::New(isolate, &SpecializedReturn<TReturn>::template method<TSelf>, v8::Uint32::NewFromUnsigned(isolate, fp.add(func))));
        }

        template<typename TSelf, typename TReturn, typename... TArgs, size_t N>
        static void with(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate>& prototype, internal::FunctionPointers& fp, TReturn (TSelf::*func)(TArgs...) const, const char (&name)[N])
        {
            prototype->Set(v8::String::NewFromUtf8Literal(isolate, name), v8::FunctionTemplate::New(isolate, &SpecializedReturn<TReturn>::template method<TSelf, TArgs...>, v8::Uint32::NewFromUnsigned(isolate, fp.add(func))));
        }
    }

}

#endif
