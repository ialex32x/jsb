#ifndef JAVASCRIPT_PRIMITIVE_BINDINGS_H
#define JAVASCRIPT_PRIMITIVE_BINDINGS_H
#include "jsb_pch.h"

#ifndef JSB_WITH_STATIC_PRIMITIVE_TYPE_BINDINGS
#define JSB_WITH_STATIC_PRIMITIVE_TYPE_BINDINGS 0
#endif

namespace jsb
{
    struct FBindingEnv
    {
        class Environment* environment;
        class Realm* realm;
        StringName type_name;

        v8::Isolate* isolate;
        const v8::Local<v8::Context>& context;

        internal::CFunctionPointers& function_pointers;
    };

    typedef NativeClassID (*PrimitiveTypeRegisterFunc)(const FBindingEnv& p_env);

    void register_primitive_bindings(class Realm* p_realm);
}
#endif
