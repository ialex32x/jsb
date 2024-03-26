#include "jsb_gdjs_lang.h"

#include <iterator>

#include "../internal/jsb_path_util.h"
#include "jsb_gdjs_script_instance.h"
#include "jsb_gdjs_script.h"

GodotJSScriptLanguage *GodotJSScriptLanguage::singleton_ = nullptr;

GodotJSScriptLanguage::GodotJSScriptLanguage()
{
    jsb_check(!singleton_);
    singleton_ = this;
}

GodotJSScriptLanguage::~GodotJSScriptLanguage()
{
    jsb_check(singleton_ == this);
    singleton_ = nullptr;
}

void GodotJSScriptLanguage::init()
{
    if (!once_inited_)
    {
        once_inited_ = true;
        runtime_ = std::make_shared<jsb::JavaScriptRuntime>();
        context_ = std::make_shared<jsb::JavaScriptContext>(runtime_);

        //TODO add GodotJSScriptLanguage itself as a module resolver which load source directly from GodotJSScript.get_source_code()
        runtime_->add_module_resolver<jsb::DefaultModuleResolver>()
            .add_search_path("res://")
            // search path for editor only scripts
            .add_search_path(jsb::internal::PathUtil::combine(
                jsb::internal::PathUtil::dirname(::OS::get_singleton()->get_executable_path()),
                "../modules/jsb/scripts/out"));

        //TODO experimental codes about handling native class binding
        context_->expose_temp();

        // editor entry script
        context_->load("main_entry");
        // context_->load("res://src/main.js");
        // runtime_->find_gdjs_class("TestClass");
    }
    JSB_LOG(Verbose, "jsb lang init");
}

void GodotJSScriptLanguage::finish()
{
    jsb_check(once_inited_);
    once_inited_ = false;
    context_.reset();
    runtime_.reset();
    JSB_LOG(Verbose, "jsb lang finish");
}

void GodotJSScriptLanguage::frame()
{
    runtime_->update();
    // runtime_->gc();
}

void GodotJSScriptLanguage::get_reserved_words(List<String>* p_words) const
{
    static const char* keywords[] = {
        "return", "function", "interface", "class", "let", "break", "as", "any", "switch", "case", "if", "enum",
        "throw", "else", "var", "number", "string", "get", "module", "instanceof", "typeof", "public", "private",
        "while", "void", "null", "super", "this", "new", "in", "await", "async", "extends", "static",
        "package", "implements", "interface", "continue", "yield", "const", "export", "finally", "for",
    };
    for (int i = 0, n = std::size(keywords); i < n; ++i)
    {
        p_words->push_back(keywords[i]);
    }
}

struct JavaScriptControlFlowKeywords
{
    HashSet<String> values;
    jsb_force_inline JavaScriptControlFlowKeywords()
    {
        constexpr static const char* _keywords[] =
        {
            "if", "else", "switch", "case", "do", "while", "for", "foreach",
            "return", "break", "continue",
            "try", "throw", "catch", "finally",
        };
        for (size_t index = 0; index < ::std::size(_keywords); ++index)
        {
            values.insert(_keywords[index]);
        }
    }
};

bool GodotJSScriptLanguage::is_control_flow_keyword(String p_keyword) const
{
    static JavaScriptControlFlowKeywords collection;
    return collection.values.has(p_keyword);
}

void GodotJSScriptLanguage::get_doc_comment_delimiters(List<String>* p_delimiters) const
{
    p_delimiters->push_back("///");
}

void GodotJSScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const
{
    p_delimiters->push_back("//");
    p_delimiters->push_back("/* */");
}

void GodotJSScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const
{
    p_delimiters->push_back("' '");
    p_delimiters->push_back("\" \"");
    p_delimiters->push_back("` `");
}

Script* GodotJSScriptLanguage::create_script() const
{
    return memnew(GodotJSScript);
}

bool GodotJSScriptLanguage::validate(const String& p_script, const String& p_path, List<String>* r_functions, List<ScriptError>* r_errors, List<Warning>* r_warnings, HashSet<int>* r_safe_lines) const
{
    jsb::JavaScriptExceptionInfo exception_info;
    if (context_->validate(p_path, &exception_info))
    {
        return true;
    }

    //TODO parse error info
    ScriptError err;
    err.line = 0;
    err.column = 0;
    err.message = exception_info;
    r_errors->push_back(err);
    return false;
}

Ref<Script> GodotJSScriptLanguage::make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const
{
    Ref<GodotJSScript> spt;
    spt.instantiate();
    spt->set_source_code(p_template);
    return spt;
}

Vector<ScriptLanguage::ScriptTemplate> GodotJSScriptLanguage::get_built_in_templates(StringName p_object)
{
    Vector<ScriptTemplate> templates;
#ifdef TOOLS_ENABLED
    //TODO load templates from disc
    ScriptTemplate st;
    st.content = "// template\nexport default class {\n}\n";
    st.description = "a javascript boilerplate";
    st.inherit = p_object;
    st.name = "Basic Class Template";
    templates.append(st);
#endif
    return templates;
}

void GodotJSScriptLanguage::reload_all_scripts()
{
    //TODO
    JSB_LOG(Verbose, "TODO");
}

void GodotJSScriptLanguage::reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload)
{
    //TODO
    JSB_LOG(Verbose, "TODO");
}

void GodotJSScriptLanguage::get_recognized_extensions(List<String>* p_extensions) const
{
    p_extensions->push_back(JSB_RES_EXT);
}
