#ifndef JAVASCRIPT_PRIMITIVE_BINDINGS_H
#define JAVASCRIPT_PRIMITIVE_BINDINGS_H
#include "jsb_pch.h"

namespace jsb
{
    struct FBindingEnv
    {
        class Environment* environment;
        class Realm* realm;

        v8::Isolate* isolate;
        const v8::Local<v8::Context>& context;

        internal::CFunctionPointers& function_pointers;
    };

    typedef NativeClassID (*PrimitiveTypeRegisterFunc)(const FBindingEnv& p_env);

    void register_primitive_bindings(class Realm* p_realm);
}
#endif
