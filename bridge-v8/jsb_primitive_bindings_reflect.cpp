#include "jsb_primitive_bindings.h"
#if !JSB_WITH_STATIC_PRIMITIVE_TYPE_BINDINGS

#include "jsb_class_info.h"
#include "jsb_transpiler.h"
#include "jsb_v8_helper.h"

#define RegisterPrimitiveType(TypeName) register_primitive_binding(GetTypeInfo<TypeName>::VARIANT_TYPE, &VariantBind<TypeName>::reflect_bind)

namespace jsb
{
    static const char* _op_names[] = {
		"op_EQUAL",            // Equality operator ([code]==[/code]).
		"op_NOT_EQUAL",            // Inequality operator ([code]!=[/code]).
		"op_LESS",             // Less than operator ([code]&lt;[/code]).
		"op_LESS_EQUAL",           // Less than or equal operator ([code]&lt;=[/code]).
		"op_GREATER",          // Greater than operator ([code]&gt;[/code]).
		"op_GREATER_EQUAL",            // Greater than or equal operator ([code]&gt;=[/code]).
		"op_ADD",          // Addition operator ([code]+[/code]).
		"op_SUBTRACT",             // Subtraction operator ([code]-[/code]).
		"op_MULTIPLY",             // Multiplication operator ([code]*[/code]).
		"op_DIVIDE",           // Division operator ([code]/[/code]).
		"op_NEGATE",           // Unary negation operator ([code]-[/code]).
		"op_POSITIVE",             // Unary plus operator ([code]+[/code]).
		"op_MODULE",           // Remainder/modulo operator ([code]%[/code]).
		"op_POWER",            // Power operator ([code]**[/code]).
		"op_SHIFT_LEFT",           // Left shift operator ([code]&lt;&lt;[/code]).
		"op_SHIFT_RIGHT",          // Right shift operator ([code]&gt;&gt;[/code]).
		"op_BIT_AND",          // Bitwise AND operator ([code]&amp;[/code]).
		"op_BIT_OR",           // Bitwise OR operator ([code]|[/code]).
		"op_BIT_XOR",          // Bitwise XOR operator ([code]^[/code]).
		"op_BIT_NEGATE",           // Bitwise NOT operator ([code]~[/code]).
		"op_AND",          // Logical AND operator ([code]and[/code] or [code]&amp;&amp;[/code]).
		"op_OR",           // Logical OR operator ([code]or[/code] or [code]||[/code]).
		"op_XOR",          // Logical XOR operator (not implemented in GDScript).
		"op_NOT",          // Logical NOT operator ([code]not[/code] or [code]![/code]).
		"op_IN",           // Logical IN operator ([code]in[/code]).
    };

    struct FMethodInfo
    {
        StringName name;
	    Vector<Variant::Type> argument_types;
        Variant::Type return_type;
    };

    template<typename T>
    struct VariantBind
    {
        constexpr static Variant::Type TYPE = GetTypeInfo<T>::VARIANT_TYPE;

        static Vector<FMethodInfo> builtin_method_collection_;

        jsb_force_inline static const FMethodInfo& get_method(int p_index)
        {
            jsb_check(p_index >= 0 && p_index < builtin_method_collection_.size());
            return builtin_method_collection_[p_index];
        }

        jsb_force_inline static int register_type_method(const StringName& p_name)
        {
            const int collection_index = builtin_method_collection_.size();
            builtin_method_collection_.append({});
            FMethodInfo& method_info = builtin_method_collection_.write[collection_index];
            method_info.name = p_name;
            method_info.return_type = Variant::get_builtin_method_return_type(TYPE, p_name);
            const int argument_count = Variant::get_builtin_method_argument_count(TYPE, p_name);
            method_info.argument_types.resize_zeroed(argument_count);
            for (int argument_index = 0; argument_index < argument_count; ++argument_index)
            {
                const Variant::Type type = Variant::get_builtin_method_argument_type(TYPE, p_name, argument_index);
                method_info.argument_types.write[argument_index] = type;
            }
            return collection_index;
        }

        static void _get_constant_value_lazy(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            const StringName constant = V8Helper::to_string(isolate, name);
            bool r_valid;
            const Variant constant_value = Variant::get_constant_value(TYPE, constant, &r_valid);
            jsb_check(r_valid);
            v8::Local<v8::Value> rval;
            if (!Realm::gd_var_to_js(isolate, context, constant_value, rval))
            {
                isolate->ThrowError("bad translate");
                return;
            }
            info.GetReturnValue().Set(rval);
        }

        static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Object> self = info.This();
            internal::Index32 class_id(v8::Local<v8::Uint32>::Cast(info.Data())->Value());
            Environment* runtime = Environment::wrap(isolate);
            const int argc = info.Length();

            const int constructor_count = Variant::get_constructor_count(TYPE);
            for (int constructor_index = 0; constructor_index < constructor_count; ++constructor_index)
            {
                if (Variant::get_constructor_argument_count(TYPE, constructor_index) != argc) continue;
                bool argument_type_match = true;
                for (int argument_index = 0; argument_index < argc; ++argument_index)
                {
                    Variant::Type argument_type = Variant::get_constructor_argument_type(TYPE, constructor_index, argument_index);
                    if (!V8Helper::can_convert_strict(info[argument_index], argument_type))
                    {
                        argument_type_match = false;
                        break;
                    }
                }

                if (!argument_type_match)
                {
                    continue;
                }

                const Variant** argv = jsb_stackalloc(const Variant*, argc);
                Variant* args = jsb_stackalloc(Variant, argc);
                for (int index = 0; index < argc; ++index)
                {
                    memnew_placement(&args[index], Variant);
                    argv[index] = &args[index];
                    Variant::Type argument_type = Variant::get_constructor_argument_type(TYPE, constructor_index, index);
                    if (!Realm::js_to_gd_var(isolate, context, info[index], argument_type, args[index]))
                    {
                        // revert all constructors
                        const CharString raw_string = vformat("bad argument: %d", index).ascii();
                        while (index >= 0) { args[index--].~Variant(); }
                        v8::Local<v8::String> error_message = v8::String::NewFromOneByte(isolate, (const uint8_t*) raw_string.ptr(), v8::NewStringType::kNormal, raw_string.length()).ToLocalChecked();
                        isolate->ThrowError(error_message);
                        return;
                    }
                }

                Variant* instance = memnew(Variant);
                Callable::CallError err;
                Variant::construct(TYPE, *instance, argv, argc, err);

                // don't forget to destruct all stack allocated variants
                for (int index = 0; index < argc; ++index)
                {
                    args[index].~Variant();
                }

                if (err.error != Callable::CallError::CALL_OK)
                {
                    memdelete(instance);
                    isolate->ThrowError("bad call");
                    return;
                }

                runtime->bind_object(class_id, instance, self);
                return;
            }

            isolate->ThrowError("bad params");
        }

        static void finalizer(Environment* runtime, void* pointer, bool p_persistent)
        {
            Variant* self = (Variant*) pointer;
            jsb_check(self->get_type() == TYPE);
            if (!p_persistent)
            {
                memdelete(self);
            }
        }

        static void _getter(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            const Variant* p_self = (Variant*) info.This().As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            const Variant::ValidatedGetter getter = (Variant::ValidatedGetter) info.Data().As<v8::External>()->Value();

            Variant value;
            getter(p_self, &value);
            v8::Local<v8::Value> rval;
            if (!Realm::gd_var_to_js(isolate, context, value, rval))
            {
                isolate->ThrowError("bad translate");
                return;
            }
            info.GetReturnValue().Set(rval);
        }

        static void _setter(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            Variant* p_self = (Variant*) info.This().As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            const Variant::ValidatedSetter setter = (Variant::ValidatedSetter) info.Data().As<v8::External>()->Value();

            Variant value;
            if (!Realm::js_to_gd_var(isolate, context, info[0], value))
            {
                isolate->ThrowError("bad translate");
                return;
            }
            setter(p_self, &value);
        }

        static void _instance_method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            Variant* self = (Variant*) info.This().As<v8::Object>()->GetAlignedPointerFromInternalField(kObjectFieldPointer);
            const FMethodInfo& method_info = get_method(info.Data().As<v8::Int32>()->Value());
            const int argc = info.Length();

            // prepare argv
            jsb_check(argc <= method_info.argument_types.size());
            const Variant** argv = jsb_stackalloc(const Variant*, argc);
            Variant* args = jsb_stackalloc(Variant, argc);
            for (int index = 0; index < argc; ++index)
            {
                memnew_placement(&args[index], Variant);
                argv[index] = &args[index];
                Variant::Type type = method_info.argument_types[index];
                if (!Realm::js_to_gd_var(isolate, context, info[index], type, args[index]))
                {
                    // revert all constructors
                    const CharString raw_string = vformat("bad argument: %d", index).ascii();
                    while (index >= 0) { args[index--].~Variant(); }
                    v8::Local<v8::String> error_message = v8::String::NewFromOneByte(isolate, (const uint8_t*) raw_string.ptr(), v8::NewStringType::kNormal, raw_string.length()).ToLocalChecked();
                    isolate->ThrowError(error_message);
                    return;
                }
            }

            // call godot method
            Callable::CallError error;
            Variant crval;
            self->callp(method_info.name, argv, argc, crval, error);

            // don't forget to destruct all stack allocated variants
            for (int index = 0; index < argc; ++index)
            {
                args[index].~Variant();
            }

            if (error.error != Callable::CallError::CALL_OK)
            {
                isolate->ThrowError("failed to call");
                return;
            }
            v8::Local<v8::Value> jrval;
            if (Realm::gd_var_to_js(isolate, context, crval, jrval))
            {
                info.GetReturnValue().Set(jrval);
                return;
            }
            isolate->ThrowError("failed to translate godot variant to v8 value");
        }

        static void _static_method(const v8::FunctionCallbackInfo<v8::Value>& info)
        {
            v8::Isolate* isolate = info.GetIsolate();
            v8::HandleScope handle_scope(isolate);
            v8::Isolate::Scope isolate_scope(isolate);
            v8::Local<v8::Context> context = isolate->GetCurrentContext();
            const FMethodInfo& method_info = get_method(info.Data().As<v8::Int32>()->Value());
            const int argc = info.Length();

            // prepare argv
            jsb_check(argc <= method_info.argument_types.size());
            const Variant** argv = jsb_stackalloc(const Variant*, argc);
            Variant* args = jsb_stackalloc(Variant, argc);
            for (int index = 0; index < argc; ++index)
            {
                memnew_placement(&args[index], Variant);
                argv[index] = &args[index];
                Variant::Type type = method_info.argument_types[index];
                if (!Realm::js_to_gd_var(isolate, context, info[index], type, args[index]))
                {
                    // revert all constructors
                    const CharString raw_string = vformat("bad argument: %d", index).ascii();
                    while (index >= 0) { args[index--].~Variant(); }
                    v8::Local<v8::String> error_message = v8::String::NewFromOneByte(isolate, (const uint8_t*) raw_string.ptr(), v8::NewStringType::kNormal, raw_string.length()).ToLocalChecked();
                    isolate->ThrowError(error_message);
                    return;
                }
            }

            // call godot method
            Callable::CallError error;
            Variant crval;
            Variant::call_static(TYPE, method_info.name, argv, argc, crval, error);

            // don't forget to destruct all stack allocated variants
            for (int index = 0; index < argc; ++index)
            {
                args[index].~Variant();
            }

            if (error.error != Callable::CallError::CALL_OK)
            {
                isolate->ThrowError("failed to call");
                return;
            }
            v8::Local<v8::Value> jrval;
            if (Realm::gd_var_to_js(isolate, context, crval, jrval))
            {
                info.GetReturnValue().Set(jrval);
                return;
            }
            isolate->ThrowError("failed to translate godot variant to v8 value");
        }

        static NativeClassID reflect_bind(const FBindingEnv& p_env)
        {
            NativeClassID class_id;
            const StringName& class_name = p_env.type_name;
            NativeClassInfo& class_info = p_env.environment->add_class(NativeClassInfo::GodotPrimitive, class_name, &class_id);

            v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(p_env.isolate, &constructor, v8::Uint32::NewFromUnsigned(p_env.isolate, class_id));
            function_template->InstanceTemplate()->SetInternalFieldCount(kObjectFieldCount);
            class_info.finalizer = &finalizer;
            class_info.template_.Reset(p_env.isolate, function_template);
            function_template->SetClassName(V8Helper::to_string(p_env.isolate, class_info.name));
            v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

            // setgets
            {
                List<StringName> members;
                Variant::get_member_list(TYPE, &members);
                for (const StringName& name : members)
                {
                    prototype_template->SetAccessorProperty(V8Helper::to_string(p_env.isolate, name),
                        v8::FunctionTemplate::New(p_env.isolate, _getter, v8::External::New(p_env.isolate, (void*) Variant::get_member_validated_getter(TYPE, name))),
                        v8::FunctionTemplate::New(p_env.isolate, _setter, v8::External::New(p_env.isolate, (void*) Variant::get_member_validated_setter(TYPE, name))));
                }
            }

            // methods
            {
                List<StringName> methods;
                Variant::get_builtin_method_list(TYPE, &methods);
                for (const StringName& name : methods)
                {
                    const int index = register_type_method(name);
                    if (Variant::is_builtin_method_static(TYPE, name))
                    {
                        function_template->Set(V8Helper::to_string(p_env.isolate, name), v8::FunctionTemplate::New(p_env.isolate, _static_method, v8::Int32::New(p_env.isolate, index)));
                    }
                    else
                    {
                        prototype_template->Set(V8Helper::to_string(p_env.isolate, name), v8::FunctionTemplate::New(p_env.isolate, _instance_method, v8::Int32::New(p_env.isolate, index)));
                    }
                }
            }

            // operators
            {
                //TODO expose operators
                //TODO Godot do not make operator list public, so we can only do it manually
                // Variant::get_operator_return_type(Variant::OP_ADD, left, right);
            }

            // enums
            HashSet<StringName> enum_constants;
            {
                List<StringName> enums;
                Variant::get_enums_for_type(TYPE, &enums);
                for (const StringName& enum_name : enums)
                {
                    List<StringName> enumerations;
                    v8::Local<v8::ObjectTemplate> enum_obj = v8::ObjectTemplate::New(p_env.isolate);
                    function_template->Set(V8Helper::to_string(p_env.isolate, enum_name), enum_obj);
                    Variant::get_enumerations_for_enum(TYPE, enum_name, &enumerations);
                    for (const StringName& enumeration : enumerations)
                    {
                        bool r_valid;
                        const int enum_value = Variant::get_enum_value(TYPE, enum_name, enumeration, &r_valid);
                        jsb_check(r_valid);
                        enum_obj->Set(V8Helper::to_string(p_env.isolate, enumeration), v8::Int32::New(p_env.isolate, enum_value));
                        enum_constants.insert(enumeration);
                    }
                }
            }

            // constants
            {
                List<StringName> constants;
                Variant::get_constants_for_type(TYPE, &constants);
                for (const StringName& constant : constants)
                {
                    // exclude all enum constants
                    if (enum_constants.has(constant)) continue;
                    function_template->SetLazyDataProperty(V8Helper::to_string(p_env.isolate, constant), _get_constant_value_lazy);
                }
            }

            return class_id;
        }
    };

    void register_primitive_bindings(class Realm* p_realm)
    {
        p_realm->RegisterPrimitiveType(Vector2);
        p_realm->RegisterPrimitiveType(Vector2i);
        p_realm->RegisterPrimitiveType(Rect2);
        p_realm->RegisterPrimitiveType(Rect2i);
        p_realm->RegisterPrimitiveType(Vector3);
        p_realm->RegisterPrimitiveType(Vector3i);
        p_realm->RegisterPrimitiveType(Transform2D);
        p_realm->RegisterPrimitiveType(Vector4);
        p_realm->RegisterPrimitiveType(Vector4i);
        p_realm->RegisterPrimitiveType(Plane);
        p_realm->RegisterPrimitiveType(Quaternion);
        p_realm->RegisterPrimitiveType(AABB);
        p_realm->RegisterPrimitiveType(Basis);
        p_realm->RegisterPrimitiveType(Transform3D);
        p_realm->RegisterPrimitiveType(Projection);
        p_realm->RegisterPrimitiveType(Color);
        // - StringName
        // - NodePath
        p_realm->RegisterPrimitiveType(RID);
        // - Object
        p_realm->RegisterPrimitiveType(Callable);
        p_realm->RegisterPrimitiveType(Signal);
        p_realm->RegisterPrimitiveType(Dictionary);
        p_realm->RegisterPrimitiveType(Array);
        p_realm->RegisterPrimitiveType(PackedByteArray);
        p_realm->RegisterPrimitiveType(PackedInt32Array);
        p_realm->RegisterPrimitiveType(PackedInt64Array);
        p_realm->RegisterPrimitiveType(PackedFloat32Array);
        p_realm->RegisterPrimitiveType(PackedFloat64Array);
        p_realm->RegisterPrimitiveType(PackedStringArray);
        p_realm->RegisterPrimitiveType(PackedVector2Array);
        p_realm->RegisterPrimitiveType(PackedVector3Array);
        p_realm->RegisterPrimitiveType(PackedColorArray);
    }

    template<typename T>
    Vector<FMethodInfo> VariantBind<T>::builtin_method_collection_;
}
#endif
