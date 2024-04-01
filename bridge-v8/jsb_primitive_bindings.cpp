#include "jsb_primitive_bindings.h"

#include "jsb_class_info.h"
#include "jsb_transpiler.h"
#include "jsb_typealias.h"

#define JSB_GENERATE_PROP_ACCESSORS(ClassName, FieldType, FieldName) \
    jsb_force_inline static FieldType ClassName##_##FieldName##_getter(ClassName* self) { return self->FieldName; } \
    jsb_force_inline static void ClassName##_##FieldName##_setter(ClassName* self, FieldType FieldName) { self->FieldName = FieldName; }

namespace jsb
{
    JSB_GENERATE_PROP_ACCESSORS(Vector2, real_t, x);
    JSB_GENERATE_PROP_ACCESSORS(Vector2, real_t, y);

    JSB_GENERATE_PROP_ACCESSORS(Vector3, real_t, x);
    JSB_GENERATE_PROP_ACCESSORS(Vector3, real_t, y);
    JSB_GENERATE_PROP_ACCESSORS(Vector3, real_t, z);

    JSB_GENERATE_PROP_ACCESSORS(Vector4, real_t, x);
    JSB_GENERATE_PROP_ACCESSORS(Vector4, real_t, y);
    JSB_GENERATE_PROP_ACCESSORS(Vector4, real_t, z);
    JSB_GENERATE_PROP_ACCESSORS(Vector4, real_t, w);

    v8::Local<v8::Value> bind_Vector2(const FBindingEnv& p_env)
    {
        NativeClassID class_id;
        const StringName class_name = jsb_typename(Vector2);
        NativeClassInfo& class_info = p_env.cruntime->add_primitive_class(Variant::VECTOR2, class_name, &class_id);

        v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector2>::create<real_t, real_t>(p_env.isolate, class_id, class_info);
        v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

        // methods
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector2, dot));
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector2, move_toward));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector2_x_getter, Vector2_x_setter, jsb_nameof(Vector2, x));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector2_y_getter, Vector2_y_setter, jsb_nameof(Vector2, y));

        return function_template->GetFunction(p_env.context).ToLocalChecked();
    }

    v8::Local<v8::Value> bind_Vector3(const FBindingEnv& p_env)
    {
        NativeClassID class_id;
        const StringName class_name = jsb_typename(Vector3);
        NativeClassInfo& class_info = p_env.cruntime->add_primitive_class(Variant::VECTOR3, class_name, &class_id);

        v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector3>::create<real_t, real_t, real_t>(p_env.isolate, class_id, class_info);
        v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

        // methods
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector3, dot));
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector3, move_toward));
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector3, octahedron_encode));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector3_x_getter, Vector3_x_setter, jsb_nameof(Vector3, x));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector3_y_getter, Vector3_y_setter, jsb_nameof(Vector3, y));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector3_z_getter, Vector3_z_setter, jsb_nameof(Vector3, z));

        return function_template->GetFunction(p_env.context).ToLocalChecked();
    }

    v8::Local<v8::Value> bind_Vector4(const FBindingEnv& p_env)
    {
        NativeClassID class_id;
        const StringName class_name = jsb_typename(Vector4);
        NativeClassInfo& class_info = p_env.cruntime->add_primitive_class(Variant::VECTOR4, class_name, &class_id);

        v8::Local<v8::FunctionTemplate> function_template = VariantClassTemplate<Vector4>::create<real_t, real_t, real_t, real_t>(p_env.isolate, class_id, class_info);
        v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();

        // methods
        bind::method(p_env.isolate, prototype_template, p_env.function_pointers, jsb_methodbind(Vector4, dot));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector4_x_getter, Vector4_x_setter, jsb_nameof(Vector4, x));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector4_y_getter, Vector4_y_setter, jsb_nameof(Vector4, y));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector4_z_getter, Vector4_z_setter, jsb_nameof(Vector4, z));
        bind::property(p_env.isolate, prototype_template, p_env.function_pointers, Vector4_w_getter, Vector4_w_setter, jsb_nameof(Vector4, w));

        return function_template->GetFunction(p_env.context).ToLocalChecked();
    }

    void register_primitive_bindings(class JavaScriptContext* p_ccontext)
    {
        p_ccontext->register_primitive_binding(jsb_typename(Vector2), bind_Vector2);
        p_ccontext->register_primitive_binding(jsb_typename(Vector3), bind_Vector3);
        p_ccontext->register_primitive_binding(jsb_typename(Vector4), bind_Vector4);
    }
}
