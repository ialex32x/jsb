#ifndef JAVASCRIPT_TEMPLATE_H
#define JAVASCRIPT_TEMPLATE_H

#include "jsb_pch.h"
#include "jsb_converter.h"
#include "jsb_object_handle.h"

namespace jsb
{
    template <typename Callee>
    struct Transpiler
    {
        template<typename ReturnType, typename... ArgumentTypes>
        static ReturnType call(Callee* callee, unsigned char* buffer, ArgumentTypes... args)
        {
            typedef ReturnType (Callee::*Functor)(ArgumentTypes...);
            Functor& func = *(Functor*) buffer;
            return (callee->*func)(args...);
        }

        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Object> self = info.This();
            v8::Local<v8::Uint32> data = v8::Local<v8::Uint32>::Cast(info.Data());
            internal::Index32 class_id(data->Value());

            Callee* ptr = new Callee;
            self->SetAlignedPointerInInternalField(kObjectFieldPointer, ptr);

            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self);
        }

        template<typename P1>
        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            v8::Local<v8::Uint32> data = v8::Local<v8::Uint32>::Cast(info.Data());
            internal::Index32 class_id(data->Value());
            if (!Converter<P1>::check(isolate, context, info[0]))
            {
                isolate->ThrowError("bad call");
                return;
            }
            P1 p1 = Converter<P1>::get(isolate, context, info[0]);
            Callee* ptr = new Callee(p1);
            self->SetAlignedPointerInInternalField(kObjectFieldPointer, ptr);

            JavaScriptRuntime* runtime = JavaScriptRuntime::wrap(isolate);
            runtime->bind_object(class_id, ptr, self);
        }

        static void finalizer(void* pointer)
        {
            Callee* callee = (Callee*) pointer;
            delete callee;
        }

        template<typename ReturnType, typename P1>
        static void dispatch(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Object> self = info.This();
            void* pointer = self->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            Callee* callee = (Callee*) pointer;

            uint8_t* func_pointer = JavaScriptContext::get_function(context, info.Data()->Uint32Value(context).ToChecked());
            const ReturnType& rval = Transpiler<Callee>::call<ReturnType, P1>(callee, func_pointer, Converter<P1>::get(isolate, context, info[0]));
            Converter<ReturnType>::set(isolate, context, info.GetReturnValue(), rval);
        }
    };

}

#endif
