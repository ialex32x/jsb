#ifndef JAVASCRIPT_FUNCTION_H
#define JAVASCRIPT_FUNCTION_H

#include "jsb_pch.h"

namespace jsb
{
    // simple wrapper of v8::Function
    // non-copyable
    struct JavaScriptFunction
    {
    private:
        friend class JavaScriptContext;
        v8::Global<v8::Function> function_;

    public:
        jsb_force_inline ~JavaScriptFunction() { function_.Reset(); }

        jsb_force_inline JavaScriptFunction(v8::Global<v8::Function>&& p_func) : function_(std::move(p_func)) {}
        jsb_force_inline JavaScriptFunction() = default;
        jsb_force_inline JavaScriptFunction(JavaScriptFunction&& p_other) noexcept = default;
        jsb_force_inline JavaScriptFunction& operator=(JavaScriptFunction&& p_other) = default;

        jsb_force_inline bool operator!() const { return !function_.IsEmpty(); }

    };
}
#endif
