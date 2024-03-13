#ifndef JAVASCRIPT_OBJECT_H
#define JAVASCRIPT_OBJECT_H

#include "jsb_pch.h"

namespace jsb
{
    struct JavaScriptObject
    {
        ~JavaScriptObject()
        {
            object_.Reset();
        }

        v8::Global<v8::Object> object_;
    };
}

#endif
