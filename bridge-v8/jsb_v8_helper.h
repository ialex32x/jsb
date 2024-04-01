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
    };
}
#endif
