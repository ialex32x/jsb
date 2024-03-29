#ifndef JAVASCRIPT_EXCEPTION_INFO_H
#define JAVASCRIPT_EXCEPTION_INFO_H

#include "jsb_pch.h"
#include "jsb_runtime.h"

namespace jsb
{
    struct JavaScriptExceptionInfo
    {
        bool handled_;
        String message_;
        // String stack;

        jsb_force_inline operator bool() const { return handled_; }

        jsb_force_inline operator String() const { return message_; }

        JavaScriptExceptionInfo() : handled_(false) {}
        JavaScriptExceptionInfo(JavaScriptExceptionInfo&&) = default;
        JavaScriptExceptionInfo& operator=(JavaScriptExceptionInfo&&) = default;

        JavaScriptExceptionInfo(v8::Isolate* isolate, const v8::TryCatch& p_catch) : handled_(false)
        {
            if (!p_catch.HasCaught())
            {
                return;
            }

            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Local<v8::Message> message = p_catch.Message();
            v8::Local<v8::Value> stack_trace;
            if (p_catch.StackTrace(context).ToLocal(&stack_trace))
            {
                v8::String::Utf8Value stack_trace_utf8(isolate, stack_trace);
                if (stack_trace_utf8.length() != 0)
                {
                    handled_ = true;
                    JavaScriptRuntime* cruntime = JavaScriptRuntime::wrap(isolate);
                    message_ = cruntime->handle_source_map(String(*stack_trace_utf8, stack_trace_utf8.length()));
                    return;
                }
            }

            // fallback to plain message
            const v8::String::Utf8Value message_utf8(isolate, message->Get());
            handled_ = true;
            message_ = String::utf8(*message_utf8, message_utf8.length());
        }
    };

}

#endif
