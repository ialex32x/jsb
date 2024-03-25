#include "jsb_gdjs_script.h"
#include "jsb_gdjs_lang.h"
#include "jsb_gdjs_script_instance.h"

bool GodotJSScript::can_instantiate() const {
#ifdef TOOLS_ENABLED
    return valid_ && (tool_ || ScriptServer::is_scripting_enabled());
#else
    return valid;
#endif
}

void GodotJSScript::set_source_code(const String &p_code)
{
    if (source_ == p_code)
    {
        return;
    }
    source_ = p_code;
}

Ref<Script> GodotJSScript::get_base_script() const
{
    //TODO
    //if (base_)
    //{
    //    return Ref(base_);
    //}
    return {};
}

StringName GodotJSScript::get_global_name() const
{
    //TODO
    return {};
}

bool GodotJSScript::inherits_script(const Ref<Script> &p_script) const
{
    const Ref<GodotJSScript> type_check = p_script;
    if (type_check.is_null())
    {
        return false;
    }
    const GodotJSScript *ptr = this;
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

StringName GodotJSScript::get_instance_base_type() const
{
    return get_js_class_info().native_class_name;
}

ScriptInstance* GodotJSScript::instance_create(Object *p_this)
{
    //TODO multi-thread scripting not supported for now
    GodotJSScriptLanguage* lang = GodotJSScriptLanguage::get_singleton();
    jsb::JavaScriptContext* ccontext = lang->get_context();

    GodotJSScriptInstance* instance = memnew(GodotJSScriptInstance);
    instance->owner_ = p_this;
    instance->owner_->set_script_instance(instance);
    instance->script_ = Ref(this);

    // class_info_.
    // ccontext->create_script_instance_binding(instance);
    return instance;
}

Error GodotJSScript::reload(bool p_keep_state)
{
    //TODO
    return OK;
}

#ifdef TOOLS_ENABLED
Vector<DocData::ClassDoc> GodotJSScript::get_documentation() const
{
    //TODO
    return {};
}

String GodotJSScript::get_class_icon_path() const
{
    //TODO
    return {};
}

PropertyInfo GodotJSScript::get_class_category() const
{
    return super::get_class_category();
}
#endif // TOOLS_ENABLED

bool GodotJSScript::has_method(const StringName &p_method) const
{
    return get_js_class_info().methods.has(p_method);
}

MethodInfo GodotJSScript::get_method_info(const StringName &p_method) const
{
    jsb_check(has_method(p_method));
    //TODO details?
    MethodInfo mi = {};
    mi.name = p_method;
    return mi;
}

bool GodotJSScript::is_abstract() const
{
    return get_js_class_info().flags & jsb::GodotJSClassInfo::Abstract;
}

ScriptLanguage* GodotJSScript::get_language() const
{
    return GodotJSScriptLanguage::get_singleton();
}

bool GodotJSScript::has_script_signal(const StringName &p_signal) const
{
    return get_js_class_info().signals.has(p_signal);
}

void GodotJSScript::get_script_method_list(List<MethodInfo> *p_list) const
{
    for (const KeyValue<StringName, jsb::GodotJSMethodInfo>& it : get_js_class_info().methods)
    {
        //TODO details?
        MethodInfo mi = {};
        mi.name = it.key;
        p_list->push_back(mi);
    }
}

void GodotJSScript::get_script_property_list(List<PropertyInfo> *p_list) const
{
    //TODO
    JSB_LOG(Warning, "TODO");
}

void GodotJSScript::get_script_signal_list(List<MethodInfo> *r_signals) const
{
    for (const KeyValue<StringName, jsb::GodotJSMethodInfo>& it : get_js_class_info().signals)
    {
        //TODO details?
        MethodInfo mi = {};
        mi.name = it.key;
        r_signals->push_back(mi);
    }
}

bool GodotJSScript::get_property_default_value(const StringName &p_property, Variant &r_value) const
{
    //TODO
    return false;
}

const Variant GodotJSScript::get_rpc_config() const
{
    //TODO
    return {};
}

#ifdef TOOLS_ENABLED
void GodotJSScript::_placeholder_erased(PlaceHolderScriptInstance *p_placeholder)
{
    //TODO
}
#endif

bool GodotJSScript::has_static_method(const StringName &p_method) const
{
    const HashMap<StringName, jsb::GodotJSMethodInfo>::ConstIterator& it = get_js_class_info().methods.find(p_method);
    return it != get_js_class_info().methods.end() && it->value.is_static();
}

bool GodotJSScript::instance_has(const Object *p_this) const
{
    //TODO
    return false;
}