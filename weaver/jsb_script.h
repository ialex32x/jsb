#ifndef JAVASCRIPT_SCRIPT_H
#define JAVASCRIPT_SCRIPT_H

#include "core/object/script_language.h"
#include "jsb_weaver_consts.h"
#include "jsb_bridge.h"

class JavaScript : public Script
{
    typedef Script super;
    GDCLASS(JavaScript, Script)

private:
    // fields
    bool tool_ = false;
    bool valid_ = false;
    bool reloading_ = false;

    String source_;
    String path_;
    JavaScript* base_ = nullptr;
    std::shared_ptr<jsb::JavaScriptRuntime> runtime_;
    jsb::GodotJSClassID gdjs_class_id_;

public:
    jsb_force_inline const jsb::JavaScriptClassInfo& get_js_class_info() const
    {
        return runtime_->get_gdjs_class(gdjs_class_id_);
    }

#pragma region Script Implementation
	virtual bool can_instantiate() const override;

	virtual Ref<Script> get_base_script() const override; //for script inheritance
	virtual StringName get_global_name() const override;
	virtual bool inherits_script(const Ref<Script> &p_script) const override;

	virtual StringName get_instance_base_type() const override; // this may not work in all scripts, will return empty if so
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this)  override { return nullptr; }
	virtual bool instance_has(const Object *p_this) const override;

	virtual bool has_source_code() const override { return !source_.is_empty(); }
	virtual String get_source_code() const override { return source_; }
	virtual void set_source_code(const String &p_code) override;
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual Vector<DocData::ClassDoc> get_documentation() const override;
	virtual String get_class_icon_path() const override;
	virtual PropertyInfo get_class_category() const override;
#endif // TOOLS_ENABLED

	// TODO: In the next compat breakage rename to `*_script_*` to disambiguate from `Object::has_method()`.
	virtual bool has_method(const StringName &p_method) const override;
	virtual bool has_static_method(const StringName &p_method) const override;

	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual bool is_tool() const override { return tool_; }
	virtual bool is_valid() const override { return valid_; }
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
