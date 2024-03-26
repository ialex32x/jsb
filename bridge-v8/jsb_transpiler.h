#ifndef JAVASCRIPT_TEMPLATE_H
#define JAVASCRIPT_TEMPLATE_H

#include "jsb_pch.h"
#include "jsb_runtime.h"
#include "jsb_context.h"
#include "jsb_object_handle.h"

#if DEV_ENABLED
#   define JSB_CLASS_BOILERPLATE_NAME(TemplateName, ClassName) \
    if ((const void *) (ClassName)) \
    {\
        const CharString cname = String(ClassName).utf8();\
        (TemplateName)->SetClassName(v8::String:: NewFromUtf8(isolate, cname.ptr(), v8::NewStringType::kNormal, cname.length()).ToLocalChecked());\
    } (void)0
#else
#   define JSB_CLASS_BOILERPLATE_NAME(TemplateName, ClassName) (void) 0
#endif

#define JSB_CLASS_BOILERPLATE() \
    jsb_force_inline static v8::Local<v8::FunctionTemplate> create(v8::Isolate* isolate, internal::Index32 class_id, NativeClassInfo& class_info)\
    {\
        v8::Local<v8::FunctionTemplate> template_ = v8::FunctionTemplate::New(isolate, &constructor, v8::Uint32::NewFromUnsigned(isolate, class_id));\
        template_->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);\
        class_info.finalizer = &finalizer;\
        class_info.template_.Reset(isolate, template_);\
        JSB_CLASS_BOILERPLATE_NAME(template_, class_info.name);\
        return template_;\
    }

#define JSB_CLASS_BOILERPLATE_ARGS() \
    template<typename...TArgs>\
    jsb_force_inline static v8::Local<v8::FunctionTemplate> create(v8::Isolate* isolate, internal::Index32 class_id, NativeClassInfo& class_info)\
    {\
        v8::Local<v8::FunctionTemplate> template_ =  v8::FunctionTemplate::New(isolate, &constructor<TArgs...>, v8::Uint32::NewFromUnsigned(isolate, class_id));\
        template_->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);\
        class_info.finalizer = &finalizer;\
        class_info.template_.Reset(isolate, template_);\
        JSB_CLASS_BOILERPLATE_NAME(template_, class_info.name);\
        return template_;\
    }

#define JSB_CONTEXT_BOILERPLATE() \
    v8::Isolate* isolate = info.GetIsolate();\
    v8::HandleScope handle_scope(isolate);\
    v8::Isolate::Scope isolate_scope(isolate);\
    v8::Local<v8::Context> context = isolate->GetCurrentContext();\
    Functor& func = *(Functor*) JavaScriptContext::get_function_pointer(context, info.Data()->Uint32Value(context).ToChecked());\
    (void) 0

namespace jsb
{
    template<typename> struct PrimitiveAccess {};

    template<typename> struct VariantCaster {};

    template<> struct VariantCaster<Vector2>
    {
        constexpr static Variant::Type Type = Variant::VECTOR2;
        static Vector2* from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            Variant* variant = (Variant*) p_val.As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            return VariantInternal::get_vector2(variant);
        }
    };

    template<> struct VariantCaster<Vector3>
    {
        constexpr static Variant::Type Type = Variant::VECTOR3;
        static Vector3* from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            Variant* variant = (Variant*) p_val.As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            return VariantInternal::get_vector3(variant);
        }
    };

    template<> struct VariantCaster<Vector4>
    {
        enum { Type = Variant::VECTOR4 };
        static Vector4* from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            Variant* variant = (Variant*) p_val.As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            return VariantInternal::get_vector4(variant);
        }
    };

    template<typename T>
    struct PrimitiveAccessBoilerplate
    {
        static T from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            return *VariantCaster<T>::from(context, p_val);
        }

        //TODO test
        static bool return_(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::FunctionCallbackInfo<v8::Value>& info, T val)
        {
            JavaScriptRuntime* cruntime =JavaScriptRuntime::wrap(isolate);
            const internal::Index32 class_id = cruntime->get_native_class_id(VariantCaster<T>::Type);
            const NativeClassInfo& class_info = cruntime->get_native_class(class_id);

            v8::Local<v8::FunctionTemplate> jtemplate = class_info.template_.Get(isolate);
            v8::Local<v8::Object> r_jval = jtemplate->InstanceTemplate()->NewInstance(context).ToLocalChecked();
            jsb_check(r_jval.As<v8::Object>()->InternalFieldCount() == kObjectFieldCount);

            Variant* p_cvar = memnew(Variant(val));
            // the lifecycle will be managed by javascript runtime, DO NOT DELETE it externally
            cruntime->bind_object(class_id, p_cvar, r_jval.As<v8::Object>(), false);
            info.GetReturnValue().Set(r_jval);
            return true;
        }
    };

    template<> struct PrimitiveAccess<Vector2> : PrimitiveAccessBoilerplate<Vector2> {};
    template<> struct PrimitiveAccess<Vector3> : PrimitiveAccessBoilerplate<Vector3> {};
    template<> struct PrimitiveAccess<Vector4> : PrimitiveAccessBoilerplate<Vector4> {};

    template<> struct PrimitiveAccess<const Vector2&> : PrimitiveAccessBoilerplate<Vector2> {};
    template<> struct PrimitiveAccess<const Vector3&> : PrimitiveAccessBoilerplate<Vector3> {};
    template<> struct PrimitiveAccess<const Vector4&> : PrimitiveAccessBoilerplate<Vector4> {};

    template<> struct PrimitiveAccess<real_t>
    {
        static real_t from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            return (real_t) p_val->NumberValue(context).ToChecked();
        }
        static bool return_(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::FunctionCallbackInfo<v8::Value>& info, real_t val)
        {
            info.GetReturnValue().Set(val);
            return true;
        }
    };

    template<> struct PrimitiveAccess<int32_t>
    {
        static int32_t from(const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& p_val)
        {
            return p_val->Int32Value(context).ToChecked();
        }
        static bool return_(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::FunctionCallbackInfo<v8::Value>& info, int32_t val)
        {
            info.GetReturnValue().Set(val);
            return true;
        }
    };

    // call with return
    template<typename TReturn>
    struct SpecializedReturn
    {
        template<typename TSelf>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (TSelf::*Functor)();
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            TReturn result = (p_self->*func)();
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        template<typename TSelf, typename P0>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (TSelf::*Functor)(P0);
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            TReturn result = (p_self->*func)(p0);
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        template<typename TSelf, typename P0, typename P1>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (TSelf::*Functor)(P0, P1);
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            P1 p1 = PrimitiveAccess<P1>::from(context, info[1]);
            TReturn result = (p_self->*func)(p0, p1);
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        static void function(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (*Functor)();
            JSB_CONTEXT_BOILERPLATE();

            TReturn result = (*func)();
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        template<typename P0>
        static void function(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (*Functor)(P0);
            JSB_CONTEXT_BOILERPLATE();
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            TReturn result = (*func)(p0);
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }

        template<typename TSelf>
        static void getter(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef TReturn (*Functor)(TSelf*);
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            TReturn result = (*func)(p_self);
            if (!PrimitiveAccess<TReturn>::return_(isolate, context, info, result))
            {
                isolate->ThrowError("failed to translate return value");
            }
        }
    };

    // call without return
    template<>
    struct SpecializedReturn<void>
    {
        template<typename TSelf, typename P0>
        static void method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef void (TSelf::*Functor)(P0);
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            (p_self->*func)(p0);
        }

        template<typename TSelf, typename P0>
        static void setter(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            typedef void (*Functor)(TSelf*, P0);
            JSB_CONTEXT_BOILERPLATE();

            TSelf* p_self = VariantCaster<TSelf>::from(context, info.This());
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            (*func)(p_self, p0);
        }

    };

    template<typename TSelf>
    struct ClassTemplate
    {
        JSB_CLASS_BOILERPLATE()
        JSB_CLASS_BOILERPLATE_ARGS()

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

        template<typename P0, typename P1, typename P2>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            if (info.Length() != 3)
            {
                isolate->ThrowError("bad args");
                return;
            }
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            P1 p1 = PrimitiveAccess<P1>::from(context, info[1]);
            P2 p2 = PrimitiveAccess<P2>::from(context, info[2]);
            TSelf* ptr = memnew(TSelf(p0, p1, p2));
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

    template<typename TSelf>
    struct VariantClassTemplate
    {
        JSB_CLASS_BOILERPLATE()
        JSB_CLASS_BOILERPLATE_ARGS()

        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());

            Variant* ptr = memnew(Variant);
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        template<typename P0>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            if (info.Length() != 1)
            {
                isolate->ThrowError("bad args");
                return;
            }
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            Variant* ptr = memnew(Variant(TSelf(p0)));
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        template<typename P0, typename P1>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            if (info.Length() != 2)
            {
                isolate->ThrowError("bad args");
                return;
            }
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            P1 p1 = PrimitiveAccess<P1>::from(context, info[1]);
            Variant* ptr = memnew(Variant(TSelf(p0, p1)));
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        template<typename P0, typename P1, typename P2>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            if (info.Length() != 3)
            {
                isolate->ThrowError("bad args");
                return;
            }
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            P1 p1 = PrimitiveAccess<P1>::from(context, info[1]);
            P2 p2 = PrimitiveAccess<P2>::from(context, info[2]);
            Variant* ptr = memnew(Variant(TSelf(p0, p1, p2)));
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        template<typename P0, typename P1, typename P2, typename P3>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            if (info.Length() != 4)
            {
                isolate->ThrowError("bad args");
                return;
            }
            P0 p0 = PrimitiveAccess<P0>::from(context, info[0]);
            P1 p1 = PrimitiveAccess<P1>::from(context, info[1]);
            P2 p2 = PrimitiveAccess<P2>::from(context, info[2]);
            P3 p3 = PrimitiveAccess<P3>::from(context, info[3]);
            Variant* ptr = memnew(Variant(TSelf(p0, p1, p2, p3)));
            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self, false);
        }

        static void finalizer(JavaScriptRuntime* runtime, void* pointer, bool p_persistent)
        {
            Variant* self = (Variant*) pointer;
            if (!p_persistent)
            {
                JSB_LOG(Verbose, "deleting object %s", uitos((uintptr_t) self));
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
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            const internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());

            jsb_checkf(info.IsConstructCall(), "call constructor as a regular function is not allowed");
            JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
            const NativeClassInfo& jclass_info = cruntime->get_native_class(class_id);
            jsb_check(jclass_info.type == NativeClassInfo::GodotObject);

            v8::Local<v8::Function> constructor = jclass_info.template_.Get(isolate)->GetFunction(context).ToLocalChecked();
            if (constructor == info.NewTarget())
            {
                const HashMap<StringName, ClassDB::ClassInfo>::Iterator it = ClassDB::classes.find(jclass_info.name);
                jsb_check(it != ClassDB::classes.end());
                const ClassDB::ClassInfo& gd_class_info = it->value;

                Object* gd_object = gd_class_info.creation_func();
                //NOTE IS IT A TRUTH that ref_count==1 after creation_func??
                jsb_check(!gd_object->is_ref_counted() || !((RefCounted*) gd_object)->is_referenced());
                cruntime->bind_object(class_id, gd_object, self, false);
            }
            else
            {
                JSB_LOG(Warning, "[experimental] constructing from subclass");
            }
        }

        static void finalizer(JavaScriptRuntime* runtime, void* pointer, bool p_persistent)
        {
            Object* self = (Object*) pointer;
            if (self->is_ref_counted())
            {
                // if we do not `free_instance_binding`, an error will be report on `reference_object (deref)`.
                // self->free_instance_binding(runtime);

                if (((RefCounted*) self)->unreference())
                {
                    if (!p_persistent)
                    {
                        JSB_LOG(Verbose, "deleting gd ref_counted object %s", uitos((uintptr_t) self));
                        memdelete(self);
                    }
                }
            }
            else
            {
                //TODO only delete when the object's lifecycle is fully managed by javascript
                if (!p_persistent)
                {
                    JSB_LOG(Verbose, "deleting gd object %s", uitos((uintptr_t) self));
                    memdelete(self);
                }
            }
        }
    };

    namespace bind
    {
        template<typename TSelf, typename TReturn, size_t N>
        static void property(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate>& prototype, internal::FunctionPointers& fp, TReturn (*getter)(TSelf*), void (*setter)(TSelf*, TReturn), const char (&name)[N])
        {
            prototype->SetAccessorProperty(v8::String::NewFromUtf8Literal(isolate, name),
                v8::FunctionTemplate::New(isolate, &SpecializedReturn<TReturn>::template getter<TSelf>, v8::Uint32::NewFromUnsigned(isolate, fp.add(getter))),
                v8::FunctionTemplate::New(isolate, &SpecializedReturn<void>::template setter<TSelf, TReturn>, v8::Uint32::NewFromUnsigned(isolate, fp.add(setter)))
                    );
        }

        template<typename TSelf, typename TReturn, size_t N>
        static void method(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate>& prototype, internal::FunctionPointers& fp, TReturn (TSelf::*func)(), const char (&name)[N])
        {
            prototype->Set(v8::String::NewFromUtf8Literal(isolate, name), v8::FunctionTemplate::New(isolate, &SpecializedReturn<TReturn>::template method<TSelf>, v8::Uint32::NewFromUnsigned(isolate, fp.add(func))));
        }

        template<typename TSelf, typename TReturn, typename... TArgs, size_t N>
        static void method(v8::Isolate* isolate, const v8::Local<v8::ObjectTemplate>& prototype, internal::FunctionPointers& fp, TReturn (TSelf::*func)(TArgs...) const, const char (&name)[N])
        {
            prototype->Set(v8::String::NewFromUtf8Literal(isolate, name), v8::FunctionTemplate::New(isolate, &SpecializedReturn<TReturn>::template method<TSelf, TArgs...>, v8::Uint32::NewFromUnsigned(isolate, fp.add(func))));
        }
    }

}

#endif
