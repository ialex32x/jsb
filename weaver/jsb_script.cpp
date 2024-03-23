#include "jsb_script.h"
#include "jsb_lang.h"

bool JavaScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
    return valid_ && (tool_ || ScriptServer::is_scripting_enabled());
#else
    return valid;
#endif
}

void JavaScript::set_source_code(const String &p_code)
{
    if (source_ == p_code)
    {
        return;
    }
    source_ = p_code;
}

Ref<Script> JavaScript::get_base_script() const
{
    //TODO
    //if (base_)
    //{
    //    return Ref(base_);
    //}
    return {};
}

StringName JavaScript::get_global_name() const
{
    //TODO
    return {};
}

bool JavaScript::inherits_script(const Ref<Script> &p_script) const
{
    const Ref<JavaScript> type_check = p_script;
    if (type_check.is_null())
    {
        return false;
    }
    const JavaScript *ptr = this;
    while (ptr)
    {
        if (ptr == p_script.ptr())
        {
            return true;
        }
        ptr = ptr->base_;
    }
    return false;
}

StringName JavaScript::get_instance_base_type() const
{
    //TODO
    return {};
}

ScriptInstance* JavaScript::instance_create(Object *p_this)
{
    //TODO
    return nullptr;
}

Error JavaScript::reload(bool p_keep_state)
{
    //TODO
    return OK;
}

#ifdef TOOLS_ENABLED
Vector<DocData::ClassDoc> JavaScript::get_documentation() const
{
    //TODO
    return {};
}

String JavaScript::get_class_icon_path() const
{
    //TODO
    return {};
}

PropertyInfo JavaScript::get_class_category() const
{
    return super::get_class_category();
}
#endif // TOOLS_ENABLED

bool JavaScript::has_method(const StringName &p_method) const
{
    //TODO
    return false;
}

MethodInfo JavaScript::get_method_info(const StringName &p_method) const
{
    //TODO
    return {};
}

bool JavaScript::is_abstract() const
{
    //TODO
    return true;
}

ScriptLanguage* JavaScript::get_language() const
{
    return JavaScriptLanguage::get_singleton();
}

bool JavaScript::has_script_signal(const StringName &p_signal) const
{
    //TODO
    return false;
}

void JavaScript::get_script_signal_list(List<MethodInfo> *r_signals) const
{
    //TODO
}

bool JavaScript::get_property_default_value(const StringName &p_property, Variant &r_value) const
{
    //TODO
    return false;
}

void JavaScript::get_script_method_list(List<MethodInfo> *p_list) const
{
    //TODO
}
void JavaScript::get_script_property_list(List<PropertyInfo> *p_list) const
{
    //TODO
}

const Variant JavaScript::get_rpc_config() const
{
    //TODO
    return {};
}

#ifdef TOOLS_ENABLED
void JavaScript::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder)
{
    //TODO
}
#endif

bool JavaScript::has_static_method(const StringName &p_method) const
{
    //TODO
    return false;
}

bool JavaScript::instance_has(const Object *p_this) const
{
    //TODO
    return false;
}
