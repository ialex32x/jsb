#ifndef JAVASCRIPT_PRIVATE_DATA_TYPE_H
#define JAVASCRIPT_PRIVATE_DATA_TYPE_H
#include "jsb_pch.h"
#include "../internal/jsb_sindex.h"

namespace jsb
{
    // namespace PrivateData
    // {
    //     enum Type : uint8_t
    //     {
    //         Nothing,          // unused
    //         GCObject,         // ensure to be finalized by JavaScriptRuntime before isolate disposed
    //         External,         // unmanaged
    //         PlainOldData,     // allocated with `memalloc`
    //     };
    // }

    enum : uint32_t
    {
        kObjectFieldPointer,
        kObjectFieldCount
    };

    struct ObjectHandle
    {
        internal::Index32 class_id;

        // primitive pointer to the native object
        void* pointer;

        // hook on v8 gc callback
        v8::Global<v8::Value> callback;

        //TODO
        // an optional strong reference
        v8::Global<v8::Value> ref_;
        uint32_t ref_count_;
    };
}

#endif
