#ifndef JAVASCRIPT_CALLABLE_H
#define JAVASCRIPT_CALLABLE_H
#include "../internal/jsb_macros.h"
#include "jsb_bridge.h"

class GodotJSCallableCustom : public CallableCustom
{
private:
    ObjectID object_id_;
    jsb::GodotJSFunctionID function_id_;
    jsb::ContextID context_id_;

public:
    static bool _compare_equal(const CallableCustom* p_a, const CallableCustom* p_b)
    {
        // types are already ensured by `Callable::operator==` with the comparator function pointers before calling
        const GodotJSCallableCustom* js_cc_a = (const GodotJSCallableCustom*) p_a;
        const GodotJSCallableCustom* js_cc_b = (const GodotJSCallableCustom*) p_b;
        return js_cc_a->function_id_ == js_cc_b->function_id_;
    }

    static bool _compare_less(const CallableCustom* p_a, const CallableCustom* p_b)
    {
        const GodotJSCallableCustom* js_cc_a = (const GodotJSCallableCustom*) p_a;
        const GodotJSCallableCustom* js_cc_b = (const GodotJSCallableCustom*) p_b;
        return js_cc_a->function_id_ < js_cc_b->function_id_;
    }

    GodotJSCallableCustom(ObjectID p_object_id, jsb::ContextID p_context_id, jsb::GodotJSFunctionID p_function_id)
    : object_id_(p_object_id), function_id_(p_function_id), context_id_(p_context_id)
    {}

    virtual ~GodotJSCallableCustom() override;

    virtual String get_as_text() const override;
    virtual ObjectID get_object() const override { return object_id_; }
    virtual void call(const Variant** p_arguments, int p_argcount, Variant& r_return_value, Callable::CallError& r_call_error) const override;

    virtual CompareEqualFunc get_compare_equal_func() const override { return _compare_equal; }
    virtual CompareLessFunc get_compare_less_func() const override { return _compare_less; }
    virtual uint32_t hash() const override { return function_id_.hash(); }
};

#endif
