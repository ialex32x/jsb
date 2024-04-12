#ifndef JAVASCRIPT_PRIMITIVE_BINDINGS_H
#define JAVASCRIPT_PRIMITIVE_BINDINGS_H
#include "jsb_pch.h"

namespace jsb
{
    struct FBindingEnv
    {
        class JavaScriptRuntime* cruntime;
        class JavaScriptContext* ccontext;

        v8::Isolate* isolate;
        const v8::Local<v8::Context>& context;

        internal::CFunctionPointers& function_pointers;
    };

    typedef v8::Local<v8::Value> (*PrimitiveTypeRegisterFunc)(const FBindingEnv& p_env);

    void register_primitive_bindings(class JavaScriptContext* p_ccontext);
}
#endif
