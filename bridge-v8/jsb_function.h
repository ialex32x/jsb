#ifndef JAVASCRIPT_FUNCTION_H
#define JAVASCRIPT_FUNCTION_H

#include "jsb_pch.h"

namespace jsb
{
    // simple wrapper of v8::Function
    struct JavaScriptFunction
    {
        jsb_force_inline JavaScriptFunction() = default;

        jsb_force_inline JavaScriptFunction(v8::Global<v8::Function>&& p_func) : function_(std::move(p_func))
        {
        }

        jsb_force_inline ~JavaScriptFunction()
        {
            function_.Reset();
        }

        jsb_force_inline JavaScriptFunction(JavaScriptFunction&& p_other) = default;
        jsb_force_inline JavaScriptFunction& operator=(JavaScriptFunction&& p_other) = default;

        jsb_force_inline bool operator!() const { return function_.IsEmpty(); }

        jsb_force_inline void get_function(v8::Isolate* isolate, v8::Local<v8::Function>& r_func)
        {
            r_func = function_.Get(isolate);
        }

    private:
        v8::Global<v8::Function> function_;
    };
}
#endif
