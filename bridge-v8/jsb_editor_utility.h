#ifndef JAVASCRIPT_EDITOR_UTILITY_H
#define JAVASCRIPT_EDITOR_UTILITY_H

#if TOOLS_ENABLED
#include "jsb_pch.h"
namespace jsb
{
    struct JavaScriptEditorUtility
    {
        static void _get_classes(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _get_global_constants(const v8::FunctionCallbackInfo<v8::Value>& info);
        static void _get_singletons(const v8::FunctionCallbackInfo<v8::Value>& info);
    };
}
#endif

#endif
