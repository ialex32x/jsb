#include "jsb_script.h"
#include "jsb_lang.h"
#include "jsb_script_instance.h"

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
    return class_info_.native;
}

ScriptInstance* JavaScript::instance_create(Object *p_this)
{
    //TODO
    return nullptr;
    // JavaScriptInstance* instance = memnew(JavaScriptInstance);
    // instance->owner_ = p_this;
    // instance->owner_->set_script_instance(instance);
    // instance->script_ = Ref(this);
    // JavaScriptLanguage::get_singleton()->bind_script_instance(p_this, this, instance);
    // return instance;
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
    return class_info_.methods.has(p_method);
}

MethodInfo JavaScript::get_method_info(const StringName &p_method) const
{
    jsb_check(has_method(p_method));
    //TODO details?
    MethodInfo mi = {};
    mi.name = p_method;
    return mi;
}

bool JavaScript::is_abstract() const
{
    return class_info_.flags & jsb::JavaScriptClassInfo::Abstract;
}

ScriptLanguage* JavaScript::get_language() const
{
    return JavaScriptLanguage::get_singleton();
}

bool JavaScript::has_script_signal(const StringName &p_signal) const
{
    return class_info_.signals.has(p_signal);
}

void JavaScript::get_script_method_list(List<MethodInfo> *p_list) const
{
    for (const KeyValue<StringName, jsb::JavaScriptMethodInfo>& it : class_info_.methods)
    {
        //TODO details?
        MethodInfo mi = {};
        mi.name = it.key;
        p_list->push_back(mi);
    }
}

void JavaScript::get_script_property_list(List<PropertyInfo> *p_list) const
{
    //TODO
    JSB_LOG(Warning, "TODO");
}

void JavaScript::get_script_signal_list(List<MethodInfo> *r_signals) const
{
    for (const KeyValue<StringName, jsb::JavaScriptMethodInfo>& it : class_info_.signals)
    {
        //TODO details?
        MethodInfo mi = {};
        mi.name = it.key;
        r_signals->push_back(mi);
    }
}

bool JavaScript::get_property_default_value(const StringName &p_property, Variant &r_value) const
{
    //TODO
    return false;
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
    const HashMap<StringName, jsb::JavaScriptMethodInfo>::ConstIterator& it = class_info_.methods.find(p_method);
    return it != class_info_.methods.end() && it->value.is_static();
}

bool JavaScript::instance_has(const Object *p_this) const
{
    //TODO
    return false;
}
