#ifndef JAVASCRIPT_CLASS_INFO_H
#define JAVASCRIPT_CLASS_INFO_H
#include "jsb_pch.h"

namespace jsb
{
    typedef void (*ConstructorFunc)(const v8::FunctionCallbackInfo<v8::Value>&);
    typedef void (*FinalizerFunc)(void*, bool /* p_persistent */);

    struct JavaScriptClassInfo
    {
        //TODO unnecessary?
        ConstructorFunc constructor;

        FinalizerFunc finalizer;

        //TODO if it's a godot class, this name is used as a backlink to godot class?
        // godot_object_constructor use this name to lookup classdb
        StringName name;

        //TODO
        internal::Index32 super_;
        v8::Global<v8::FunctionTemplate> template_;
    };
}

#endif
