#ifndef JAVASCRIPT_V8_HELPER_H
#define JAVASCRIPT_V8_HELPER_H
#include "jsb_pch.h"

namespace jsb
{
    namespace V8Helper
    {
        /**
         * Get a godot String with a utf-16 v8 string
         * @note crash if failed
         */
        jsb_force_inline String to_string(const v8::String::Value& p_val)
        {
            String str_gd;
            Error err = str_gd.parse_utf16((const char16_t*) *p_val, p_val.length());
            jsb_check(err == OK);
            return str_gd;
        }

        jsb_force_inline v8::MaybeLocal<v8::Script> compile(v8::Local<v8::Context> context, v8::Local<v8::String> source, const String& p_filename)
        {
            v8::Isolate* isolate = context->GetIsolate();
            if (p_filename.is_empty())
            {
                return v8::Script::Compile(context, source);
            }

#if WINDOWS_ENABLED
            const CharString filename = p_filename.replace("/", "\\").utf8();
#else
            const CharString filename = p_filename.utf8();
#endif
            v8::ScriptOrigin origin(isolate, v8::String::NewFromUtf8(isolate, filename, v8::NewStringType::kNormal, filename.length()).ToLocalChecked());
            return v8::Script::Compile(context, source, &origin);
        }
    };
}
#endif
