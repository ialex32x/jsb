#include "jsb_callable_custom.h"

String GodotJSCallableCustom::get_as_text() const
{
    //TODO a human readable string, but OK if empty
    return String();
}

GodotJSCallableCustom::~GodotJSCallableCustom()
{
    if (std::shared_ptr<jsb::JavaScriptContext> ccontext = jsb::JavaScriptContext::get_context(context_id_))
    {
        ccontext->remove_function(function_id_);
    }
}

void GodotJSCallableCustom::call(const Variant** p_arguments, int p_argcount, Variant& r_return_value, Callable::CallError& r_call_error) const
{
    std::shared_ptr<jsb::JavaScriptContext> ccontext = jsb::JavaScriptContext::get_context(context_id_);
    if (!ccontext)
    {
        r_call_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
        return;
    }

    const std::shared_ptr<jsb::JavaScriptRuntime>& cruntime = ccontext->get_runtime();
    const jsb::NativeObjectID object_id = object_id_.is_null() ? jsb::NativeObjectID() : cruntime->get_object_id(ObjectDB::get_instance(object_id_));
    ccontext->call_function(object_id, function_id_, p_arguments, p_argcount, r_call_error);
}

// bool GodotJSCallableCustom::_compare_equal(const CallableCustom* p_a, const CallableCustom* p_b)
// {
//     const GodotJSCallableCustom* js_cc_a = (const GodotJSCallableCustom*) p_a;
//     const GodotJSCallableCustom* js_cc_b = (const GodotJSCallableCustom*) p_b;
//     if (js_cc_a->context_id_ != js_cc_b->context_id_) return false;
//     if (js_cc_a->object_id_ != js_cc_b->object_id_) return false;
//     if (js_cc_a->function_id_ == js_cc_b->function_id_) return true;
//     std::shared_ptr<jsb::JavaScriptContext> ccontext = jsb::JavaScriptContext::get_context(js_cc_a->context_id_);
//     return ccontext && ccontext->equals_function(js_cc_a->function_id_, js_cc_b->function_id_);
// }
