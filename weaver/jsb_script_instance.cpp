#include "jsb_script_instance.h"
#include "jsb_lang.h"

Object* JavaScriptInstance::get_owner()
{
    //TODO
    return nullptr;
}

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
    //TODO
    return {};
}


void JavaScriptInstance::notification(int p_notification, bool p_reversed)
{
    //TODO

}


Ref<Script> JavaScriptInstance::get_script() const
{
    //TODO
    return {};
}


ScriptLanguage *JavaScriptInstance::get_language()
{
    return JavaScriptLanguage::get_singleton();
}

const Variant JavaScriptInstance::get_rpc_config() const
{
    //TODO
    return script->get_rpc_config();
}
