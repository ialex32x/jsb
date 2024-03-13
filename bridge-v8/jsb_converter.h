#ifndef JAVASCRIPT_CONVERTER_H
#define JAVASCRIPT_CONVERTER_H

namespace jsb
{
    template<typename T>
    struct Converter {};

    template<>
    struct Converter<int32_t>
    {
        static bool check(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& jsval)
        {
            return jsval->IsInt32();
        }

        static int32_t get(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Value>& jsval)
        {
            return jsval->Int32Value(context).ToChecked();
        }

        static void set(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8::ReturnValue<v8::Value>&& jsval, const int32_t& cval)
        {
            jsval.Set(cval);
        }
    };
}

#endif
