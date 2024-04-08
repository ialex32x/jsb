#ifndef JAVASCRIPT_CALLABLE_H
#define JAVASCRIPT_CALLABLE_H
#include "../internal/jsb_macros.h"
#include "jsb_bridge.h"

class GodotJSCallableCustom : public CallableCustom
{
private:
    jsb::GodotJSFunctionID function_id_;

public:
    static bool _compare_equal(const CallableCustom* p_a, const CallableCustom* p_b)
    {
        //TODO will crash if the `Callable` objects are created with different languages?
        const GodotJSCallableCustom* js_cc_a = (const GodotJSCallableCustom*) p_a;
        const GodotJSCallableCustom* js_cc_b = (const GodotJSCallableCustom*) p_b;
        return js_cc_a->function_id_ == js_cc_b->function_id_;
    }
    static bool _compare_less(const CallableCustom* p_a, const CallableCustom* p_b)
    {
        //TODO will crash if the `Callable` objects are created with different languages?
        const GodotJSCallableCustom* js_cc_a = (const GodotJSCallableCustom*) p_a;
        const GodotJSCallableCustom* js_cc_b = (const GodotJSCallableCustom*) p_b;
        return js_cc_a->function_id_ < js_cc_b->function_id_;
    }

    GodotJSCallableCustom();
    virtual ~GodotJSCallableCustom() override;

    virtual uint32_t hash() const override;
    virtual String get_as_text() const override;
    virtual ObjectID get_object() const override;
    virtual void call(const Variant **p_arguments, int p_argcount, Variant &r_return_value, Callable::CallError &r_call_error) const override;

    virtual CompareEqualFunc get_compare_equal_func() const override { return _compare_equal; }
    virtual CompareLessFunc get_compare_less_func() const override { return _compare_less; }
};

#endif
