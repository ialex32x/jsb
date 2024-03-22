#ifndef JAVASCRIPT_SCRIPT_H
#define JAVASCRIPT_SCRIPT_H

#include "core/object/script_language.h"

class JavaScript : public Script
{
    GDCLASS(JavaScript, Script)

private:
    // fields
    bool tool = false;
    bool valid = false;
    bool reloading = false;

    String source;
    String path;

public:
#pragma region Script Implementation
	virtual bool can_instantiate() const override;

	virtual Ref<Script> get_base_script() const override; //for script inheritance
	virtual StringName get_global_name() const override;
	virtual bool inherits_script(const Ref<Script> &p_script) const override;

	virtual StringName get_instance_base_type() const override; // this may not work in all scripts, will return empty if so
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this)  override { return nullptr; }
	virtual bool instance_has(const Object *p_this) const override;

	virtual bool has_source_code() const override { return !source.is_empty(); }
	virtual String get_source_code() const override { return source; }
	virtual void set_source_code(const String &p_code) override;
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual Vector<DocData::ClassDoc> get_documentation() const override;
	virtual String get_class_icon_path() const override;
	virtual PropertyInfo get_class_category() const override;
#endif // TOOLS_ENABLED

	// TODO: In the next compat breakage rename to `*_script_*` to disambiguate from `Object::has_method()`.
	virtual bool has_method(const StringName &p_method) const override;
	virtual bool has_static_method(const StringName &p_method) const override { return false; }

	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual bool is_tool() const override { return tool; }
	virtual bool is_valid() const override { return valid; }
	virtual bool is_abstract() const override;

	virtual ScriptLanguage* get_language() const override;

	virtual bool has_script_signal(const StringName &p_signal) const override;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override;

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;

	virtual void update_exports() override {} //editor tool
	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;

	virtual int get_member_line(const StringName &p_member) const override { return -1; }

	virtual void get_constants(HashMap<StringName, Variant> *p_constants) override {}
	virtual void get_members(HashSet<StringName> *p_constants) override {}

	virtual bool is_placeholder_fallback_enabled() const override { return false; }

	virtual const Variant get_rpc_config() const override;
#pragma endregion // Script Interface Implementation

protected:
#pragma region Script Implementation
#ifdef TOOLS_ENABLED
    virtual void _placeholder_erased(PlaceHolderScriptInstance *p_placeholder) override;
#endif
#pragma endregion

private:
    // methods
};

#endif
