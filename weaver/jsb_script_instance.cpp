#include "jsb_script_instance.h"
#include "jsb_lang.h"

bool JavaScriptInstance::set(const StringName &p_name, const Variant &p_value)
{
    //TODO
    return false;
}

bool JavaScriptInstance::get(const StringName &p_name, Variant &r_ret) const
{
    //TODO
    return false;
}

void JavaScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const
{
    //TODO
}

Variant::Type JavaScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const
{
    //TODO
    return Variant::NIL;
}

void JavaScriptInstance::validate_property(PropertyInfo &p_property) const
{
    //TODO
}


bool JavaScriptInstance::property_can_revert(const StringName &p_name) const
{
    //TODO
    return false;
}

bool JavaScriptInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const
{
    //TODO
    return false;
}


void JavaScriptInstance::get_method_list(List<MethodInfo> *p_list) const
{
    //TODO

}

bool JavaScriptInstance::has_method(const StringName &p_method) const
{
    //TODO
    return false;
}

Variant JavaScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error)
{

    if (const HashMap<StringName, jsb::JavaScriptFunction>::Iterator& it = cached_methods_.find(p_method))
    {
        // it->value.Get()
    }
    else
    {
        //TODO find and cache

    }

    r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
    return {};
}

void JavaScriptInstance::notification(int p_notification, bool p_reversed)
{
    //TODO find the method named `_notification`, cal it with `p_notification` as `argv`
    //TODO call it at all type levels? @seealso `GDScriptInstance::notification`
    Variant value = p_notification;
    const Variant* argv[] = { &value };
    Callable::CallError error;
    callp(SNAME("_notification"), argv, 1, error);
}

ScriptLanguage *JavaScriptInstance::get_language()
{
    return JavaScriptLanguage::get_singleton();
}
