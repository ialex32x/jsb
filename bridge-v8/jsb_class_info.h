#ifndef JAVASCRIPT_CLASS_INFO_H
#define JAVASCRIPT_CLASS_INFO_H
#include "jsb_pch.h"

namespace jsb
{
    typedef void (*ConstructorFunc)(const v8::FunctionCallbackInfo<v8::Value>&);
    typedef void (*FinalizerFunc)(class JavaScriptRuntime*, void*, bool /* p_persistent */);

    namespace JavaScriptClassType
    {
        enum Type : uint8_t
        {
            None,
            GodotObject,
            GodotPrimitive,
        };
    }

    struct JavaScriptClassInfo
    {
        // the func to release the exposed C++ (godot/variant/native) object
        // it's called when a JS value with this class type garbage collected by JS runtime
        FinalizerFunc finalizer;

        //TODO RESERVED FOR FUTURE USE
        JavaScriptClassType::Type type;

        // *only if type == GodotObject*
        // godot_object_constructor use this name to look up classdb
        StringName name;

        // strong reference
        // the counterpart of exposed C++ class.
        v8::Global<v8::FunctionTemplate> template_;
    };
}

#endif
