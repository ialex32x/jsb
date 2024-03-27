#include "jsb_editor_utility.h"

#include "core/core_constants.h"
#if TOOLS_ENABLED

#define JSB_STRINGIFY_2(a) #a
#define JSB_STRINGIFY(a) JSB_STRINGIFY_2(a)
namespace jsb_private
{
    //NOTE dummy functions only for compile-time check
    template <typename T>
    bool get_member_name(const T&);
    template <typename T>
    bool get_member_name(const volatile T&);
    template <typename R, typename... Args>
    bool get_member_name(R (*)(Args...));
}

#define JSB_GET_FIELD_NAME(TypeName, ValueName) ((void)sizeof(jsb_private::get_member_name(TypeName::ValueName)), JSB_STRINGIFY(ValueName))
#define JSB_GET_FIELD_NAME_I(InstName, ValueName) ((void)sizeof(jsb_private::get_member_name(std::decay_t<decltype(InstName)>::ValueName)), JSB_STRINGIFY(ValueName))
#define JSB_GET_FIELD_NAME_PRESET(InstName, ValueName) ((void)sizeof(jsb_private::get_member_name(std::decay_t<decltype(InstName)>::ValueName)), JSB_STRINGIFY(ValueName)), InstName.ValueName

namespace jsb
{
    namespace
    {
        v8::MaybeLocal<v8::String> S(v8::Isolate* isolate, const String& name)
        {
            const CharString char_string = name.utf8();
            return v8::String::NewFromUtf8(isolate, char_string.ptr(), v8::NewStringType::kNormal, char_string.length());
        }

        v8::MaybeLocal<v8::String> S(v8::Isolate* isolate, const char* name)
        {
            return v8::String::NewFromUtf8(isolate, name, v8::NewStringType::kNormal);
        }

        v8::MaybeLocal<v8::String> S(v8::Isolate* isolate, const StringName& name)
        {
            return S(isolate, (String) name);
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const v8::Local<v8::Value>& field_value)
        {
            obj->Set(context, v8::String::NewFromUtf8Literal(isolate, field_name), field_value).Check();
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const StringName& field_value)
        {
            set_field(isolate, context, obj, field_name, S(isolate, field_value).ToLocalChecked());
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const int32_t& field_value)
        {
            set_field(isolate, context, obj, field_name, v8::Int32::New(isolate, field_value));
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const uint32_t& field_value)
        {
            set_field(isolate, context, obj, field_name, v8::Int32::NewFromUnsigned(isolate, field_value));
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const String& field_value)
        {
            set_field(isolate, context, obj, field_name, S(isolate, field_value).ToLocalChecked());
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const char*& field_value)
        {
            set_field(isolate, context, obj, field_name, S(isolate, field_value).ToLocalChecked());
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], const bool& field_value)
        {
            set_field(isolate, context, obj, field_name, v8::Boolean::New(isolate, field_value));
        }

        template<int N>
        void set_field(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const v8::Local<v8::Object>& obj, const char (&field_name)[N], int64_t field_value)
        {
            const int32_t scaled_value = (int32_t) field_value;
            if (field_value != (int64_t) scaled_value)
            {
                JSB_LOG(Warning, "integer overflowed %s (%s) [reversible? %c]", field_name, itos(field_value), (int64_t)(double) field_value == field_value ? 'T' : 'F');
                set_field(isolate, context, obj, field_name, v8::Number::New(isolate, (double) field_value));
            }
            else
            {
                set_field(isolate, context, obj, field_name, v8::Int32::New(isolate, scaled_value));
            }
        }

        void build_property_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const PropertyInfo& property_info, const v8::Local<v8::Object>& object)
        {
            set_field(isolate, context, object, "name", property_info.name);
            set_field(isolate, context, object, "type", property_info.type);
            set_field(isolate, context, object, "class_name", property_info.class_name);
            set_field(isolate, context, object, "hint", property_info.hint);
            set_field(isolate, context, object, "hint_string", property_info.hint_string);
            set_field(isolate, context, object, "usage", property_info.usage);
        }

        void build_property_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const StringName& property_name, const ClassDB::PropertySetGet& property_info, const v8::Local<v8::Object>& object)
        {
            set_field(isolate, context, object, "name", property_name);
            set_field(isolate, context, object, "type", property_info.type);
            set_field(isolate, context, object, "setter", property_info.setter);
            set_field(isolate, context, object, "getter", property_info.getter);
        }

        void build_method_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, MethodBind const* method_bind, const v8::Local<v8::Object>& object)
        {
            set_field(isolate, context, object, "id", method_bind->get_method_id());
            set_field(isolate, context, object, "name", method_bind->get_name());
            set_field(isolate, context, object, "hint_flags", method_bind->get_hint_flags());
            set_field(isolate, context, object, "is_static", method_bind->is_static());
            set_field(isolate, context, object, "is_const", method_bind->is_const());
            set_field(isolate, context, object, "is_vararg", method_bind->is_vararg());
            // set_field(isolate, context, object, "has_return", method_bind->has_return());
            set_field(isolate, context, object, "argument_count", method_bind->get_argument_count());

            if (method_bind->has_return())
            {
                const PropertyInfo& return_info = method_bind->get_return_info();
                v8::Local<v8::Object> property_info_obj = v8::Object::New(isolate);
                build_property_info(isolate, context, return_info, property_info_obj);
                set_field(isolate, context, object, "return_", property_info_obj);
            }

            const int argc = method_bind->get_argument_count();
            v8::Local<v8::Array> args_obj = v8::Array::New(isolate, argc);
            for (int index = 0; index < argc; ++index)
            {
                const PropertyInfo& arg_info = method_bind->get_argument_info(index);
                v8::Local<v8::Object> property_info_obj = v8::Object::New(isolate);
                build_property_info(isolate, context, arg_info, property_info_obj);
                args_obj->Set(context, index, property_info_obj);
            }
            set_field(isolate, context, object, "args_", args_obj);
        }

        void build_enum_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const ClassDB::ClassInfo::EnumInfo& enum_info, const v8::Local<v8::Object>& object)
        {
            const int num = enum_info.constants.size();
            v8::Local<v8::Array> constants_obj = v8::Array::New(isolate, num);
            for (int index = 0; index < num; ++index)
            {
                const StringName& name = enum_info.constants[index];
                constants_obj->Set(context, index, S(isolate, name).ToLocalChecked());
            }
            set_field(isolate, context, object, "literals", constants_obj);
            set_field(isolate, context, object, JSB_GET_FIELD_NAME_PRESET(enum_info, is_bitfield));
        }

        void build_signal_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const MethodInfo& method_info, const v8::Local<v8::Object>& object)
        {
            set_field(isolate, context, object, "id", method_info.id);
            set_field(isolate, context, object, "name_", method_info.name);
            set_field(isolate, context, object, "flags", method_info.flags);
            // TODO
            // set_field(isolate, context, object, "return_val", method_info.return_val);
            // set_field(isolate, context, object, "arguments", method_info.arguments);
        }

        void build_class_info(v8::Isolate* isolate, const v8::Local<v8::Context>& context, const StringName& class_name, v8::Local<v8::Object>& class_info_obj)
        {
            const HashMap<StringName, ClassDB::ClassInfo>::Iterator class_it = ClassDB::classes.find(class_name);

            jsb_check(class_it != ClassDB::classes.end());
            const ClassDB::ClassInfo& class_info = class_it->value;
            set_field(isolate, context, class_info_obj, "name", class_name);
            set_field(isolate, context, class_info_obj, "super", class_info.inherits);

            {
                v8::Local<v8::Array> properties_obj = v8::Array::New(isolate, class_info.property_list.size());
                set_field(isolate, context, class_info_obj, "fields", properties_obj);
                for (int index = 0, num = class_info.property_list.size(); index < num; ++index)
                {
                    const PropertyInfo& property_info = class_info.property_list[index];
                    v8::Local<v8::Object> property_info_obj = v8::Object::New(isolate);
                    build_property_info(isolate, context, property_info, property_info_obj);
                    properties_obj->Set(context, index, property_info_obj);
                }
            }

            {
                v8::Local<v8::Array> properties_obj = v8::Array::New(isolate, (int) class_info.property_setget.size());
                set_field(isolate, context, class_info_obj, "properties", properties_obj);
                int index = 0;
                for (const KeyValue<StringName, ClassDB::PropertySetGet>& pair : class_info.property_setget)
                {
                    const StringName& property_name = pair.key;
                    const ClassDB::PropertySetGet& property_info = pair.value;
                    v8::Local<v8::Object> property_info_obj = v8::Object::New(isolate);
                    build_property_info(isolate, context, property_name, property_info, property_info_obj);
                    properties_obj->Set(context, index++, property_info_obj);
                }
            }

            {
                v8::Local<v8::Array> methods_obj = v8::Array::New(isolate, (int) class_info.method_map.size());
                set_field(isolate, context, class_info_obj, "methods", methods_obj);
                int index = 0;
                for (const KeyValue<StringName, MethodBind*>& pair : class_info.method_map)
                {
                    MethodBind const * const method_bind = pair.value;
                    v8::Local<v8::Object> method_info_obj = v8::Object::New(isolate);
                    build_method_info(isolate, context, method_bind, method_info_obj);
                    methods_obj->Set(context, index++, method_info_obj);
                }
            }

            {
                v8::Local<v8::Array> enums_obj = v8::Array::New(isolate, (int) class_info.enum_map.size());
                set_field(isolate, context, class_info_obj, "enums", enums_obj);
                int index = 0;
                for (const KeyValue<StringName, ClassDB::ClassInfo::EnumInfo>& pair : class_info.enum_map)
                {
                    const ClassDB::ClassInfo::EnumInfo& enum_info = pair.value;
                    v8::Local<v8::Object> enum_info_obj = v8::Object::New(isolate);
                    set_field(isolate, context, enum_info_obj, "name", pair.key);
                    build_enum_info(isolate, context, enum_info, enum_info_obj);
                    enums_obj->Set(context, index++, enum_info_obj);
                }
            }

            {
                v8::Local<v8::Array> constants_obj = v8::Array::New(isolate, (int) class_info.constant_map.size());
                set_field(isolate, context, class_info_obj, "constants", constants_obj);
                int index = 0;
                for (const KeyValue<StringName, int64_t>& pair : class_info.constant_map)
                {
                    v8::Local<v8::Object> constant_info_obj = v8::Object::New(isolate);
                    set_field(isolate, context, constant_info_obj, "name", pair.key);
                    set_field(isolate, context, constant_info_obj, "value", pair.value);
                    constants_obj->Set(context, index++, constant_info_obj);
                }
            }

            {
                v8::Local<v8::Array> signals_obj = v8::Array::New(isolate, (int) class_info.signal_map.size());
                set_field(isolate, context, class_info_obj, "signals", signals_obj);
                int index = 0;
                for (const KeyValue<StringName, MethodInfo>& pair : class_info.signal_map)
                {
                    v8::Local<v8::Object> signal_info_obj = v8::Object::New(isolate);
                    set_field(isolate, context, signal_info_obj, "name", pair.key);
                    build_signal_info(isolate, context, pair.value, signal_info_obj);
                    signals_obj->Set(context, index++, signal_info_obj);
                }
            }
        }
    }

    void JavaScriptEditorUtility::_get_classes(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        List<StringName> list;
        ClassDB::get_class_list(&list);

        v8::Local<v8::Array> array = v8::Array::New(isolate, list.size());
        for (int index = 0, num = list.size(); index < num; ++index)
        {
            v8::Local<v8::Object> class_info = v8::Object::New(isolate);

            build_class_info(isolate, context, list[index], class_info);
            array->Set(context, index, class_info);
        }
        info.GetReturnValue().Set(array);
    }

    void JavaScriptEditorUtility::_get_global_constants(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        const int num = CoreConstants::get_global_constant_count();
        v8::Local<v8::Array> array = v8::Array::New(isolate, num);
        for (int index = 0; index < num; ++index)
        {
            const char* name = CoreConstants::get_global_constant_name(index);
            const int64_t value = CoreConstants::get_global_constant_value(index);
            v8::Local<v8::Object> constant_obj = v8::Object::New(isolate);
            set_field(isolate, context, constant_obj, "name", name);
            set_field(isolate, context, constant_obj, "value", value);
            array->Set(context, index, constant_obj);
        }
        info.GetReturnValue().Set(array);
    }

    void JavaScriptEditorUtility::_get_singletons(const v8::FunctionCallbackInfo<v8::Value>& info)
    {
        v8::Isolate* isolate = info.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();

        List<Engine::Singleton> singletons;
        Engine::get_singleton()->get_singletons(&singletons);
        v8::Local<v8::Array> array = v8::Array::New(isolate, singletons.size());
        for (int index = 0, num = singletons.size(); index < num; ++index)
        {
            const Engine::Singleton& singleton = singletons[index];
            v8::Local<v8::Object> constant_obj = v8::Object::New(isolate);
            set_field(isolate, context, constant_obj, "name", singleton.name);
            set_field(isolate, context, constant_obj, "class_name", singleton.class_name);
            set_field(isolate, context, constant_obj, "user_created", singleton.user_created);
            set_field(isolate, context, constant_obj, "editor_only", singleton.editor_only);
            array->Set(context, index, constant_obj);
        }
        info.GetReturnValue().Set(array);
    }

}
#endif
