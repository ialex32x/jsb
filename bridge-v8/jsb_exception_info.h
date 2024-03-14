#ifndef JAVASCRIPT_EXCEPTION_INFO_H
#define JAVASCRIPT_EXCEPTION_INFO_H

#include "jsb_pch.h"

namespace jsb
{
    struct JavaScriptExceptionInfo
    {
        String message;
        // String stack;

        static bool _read_exception(v8::Isolate* isolate, const v8::TryCatch& p_catch, JavaScriptExceptionInfo& r_info)
        {
            if (p_catch.HasCaught())
            {
                v8::Local<v8::Context> context = isolate->GetCurrentContext();
                v8::Local<v8::Message> message = p_catch.Message();
                v8::Local<v8::Value> stack_trace;
                if (p_catch.StackTrace(context).ToLocal(&stack_trace))
                {
                    v8::String::Utf8Value stack_trace_utf8(isolate, stack_trace);
                    if (stack_trace_utf8.length() != 0)
                    {
                        r_info.message = String(*stack_trace_utf8, stack_trace_utf8.length());
                        return true;
                    }
                }

                // fallback to plain message
                const v8::String::Utf8Value message_utf8(isolate, message->Get());
                r_info.message = String::utf8(*message_utf8, message_utf8.length());
                return true;
            }
            return false;
        }
    };

}

#endif
