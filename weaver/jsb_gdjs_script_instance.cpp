#include "jsb_gdjs_script_instance.h"
#include "jsb_gdjs_lang.h"

bool GodotJSScriptInstance::set(const StringName &p_name, const Variant &p_value)
{
    //TODO
    return false;
}

bool GodotJSScriptInstance::get(const StringName &p_name, Variant &r_ret) const
{
    //TODO
    return false;
}

void GodotJSScriptInstance::get_property_list(List<PropertyInfo> *p_properties) const
{
    //TODO
}

Variant::Type GodotJSScriptInstance::get_property_type(const StringName &p_name, bool *r_is_valid) const
{
    //TODO
    return Variant::NIL;
}

void GodotJSScriptInstance::validate_property(PropertyInfo &p_property) const
{
    //TODO
}


bool GodotJSScriptInstance::property_can_revert(const StringName &p_name) const
{
    //TODO
    return false;
}

bool GodotJSScriptInstance::property_get_revert(const StringName &p_name, Variant &r_ret) const
{
    //TODO
    return false;
}


void GodotJSScriptInstance::get_method_list(List<MethodInfo> *p_list) const
{
    //TODO

}

bool GodotJSScriptInstance::has_method(const StringName &p_method) const
{
    //TODO
    return false;
}

Variant GodotJSScriptInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error)
{

    if (const HashMap<StringName, jsb::JavaScriptFunction>::Iterator& it = cached_methods_.find(p_method))
    {
        // it->value.Get()
    }
    else
    {
        //TODO find and cache
        //GodotJSScriptLanguage::get_singleton()->get_func_handle(object_id_, p_method);
    }

    r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
    return {};
}

void GodotJSScriptInstance::notification(int p_notification, bool p_reversed)
{
    //TODO find the method named `_notification`, cal it with `p_notification` as `argv`
    //TODO call it at all type levels? @seealso `GDScriptInstance::notification`
    Variant value = p_notification;
    const Variant* argv[] = { &value };
    Callable::CallError error;
    callp(SNAME("_notification"), argv, 1, error);
}

ScriptLanguage *GodotJSScriptInstance::get_language()
{
    return GodotJSScriptLanguage::get_singleton();
}

GodotJSScriptInstance::~GodotJSScriptInstance()
{
    MutexLock lock(GodotJSScriptLanguage::singleton_->mutex_);
    jsb_check(script_.is_valid() && owner_);
    script_->instances_.erase(owner_);
}
