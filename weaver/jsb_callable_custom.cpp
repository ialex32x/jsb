#include "jsb_callable_custom.h"

String GodotJSCallableCustom::get_as_text() const
{
    //TODO a human readable string, but OK if empty
    return String();
}

GodotJSCallableCustom::~GodotJSCallableCustom()
{
    //TODO release the function handle
}

void GodotJSCallableCustom::call(const Variant** p_arguments, int p_argcount, Variant& r_return_value, Callable::CallError& r_call_error) const
{
    //TODO
}
