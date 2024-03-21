#include "jsb_lang.h"
#include <iterator>
#include "../internal/jsb_path_util.h"

namespace jsb
{
    JavaScriptBridgeLanguage::JavaScriptBridgeLanguage()
    {
        runtime_ = std::make_shared<JavaScriptRuntime>();
        context_ = std::make_shared<JavaScriptContext>(runtime_);
    }

    JavaScriptBridgeLanguage::~JavaScriptBridgeLanguage()
    {
    }

    void JavaScriptBridgeLanguage::init()
    {
        if (!once_inited_)
        {
            once_inited_ = true;
            constexpr const char* xxx = __FILE__;
            runtime_->add_module_resolver<jsb::DefaultModuleResolver>()
                .add_search_path("res://")
                // search path for editor only scripts
                .add_search_path(jsb::internal::PathUtil::combine(
                    jsb::internal::PathUtil::dirname(::OS::get_singleton()->get_executable_path()),
                    "../modules/jsb/scripts/out"));

            // editor entry script
            context_->load("main_entry");
        }

    }

    void JavaScriptBridgeLanguage::finish()
    {
    }

    void JavaScriptBridgeLanguage::frame()
    {
        runtime_->update();
        // runtime_->gc();
    }

    void JavaScriptBridgeLanguage::get_reserved_words(List<String>* p_words) const
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

    bool JavaScriptBridgeLanguage::is_control_flow_keyword(String p_keywords) const
    {
        return
            p_keywords == "if" ||
            p_keywords == "else" ||
            p_keywords == "switch" ||
            p_keywords == "case" ||
            p_keywords == "do" ||
            p_keywords == "while" ||
            p_keywords == "for" ||
            p_keywords == "foreach" ||
            p_keywords == "return" ||
            p_keywords == "break" ||
            p_keywords == "continue" ||
            p_keywords == "try" ||
            p_keywords == "throw" ||
            p_keywords == "catch" ||
            p_keywords == "finally";
    }

    void JavaScriptBridgeLanguage::get_doc_comment_delimiters(List<String>* p_delimiters) const
    {
        p_delimiters->push_back("///");
    }

    void JavaScriptBridgeLanguage::get_comment_delimiters(List<String> *p_delimiters) const
    {
        p_delimiters->push_back("//");
        p_delimiters->push_back("/* */");
    }

    void JavaScriptBridgeLanguage::get_string_delimiters(List<String> *p_delimiters) const
    {
        p_delimiters->push_back("' '");
        p_delimiters->push_back("\" \"");
        p_delimiters->push_back("` `");
    }

    Script* JavaScriptBridgeLanguage::create_script() const
    {
        //TODO
        return nullptr;
    }

    Error JavaScriptBridgeLanguage::execute_file(const String& p_path)
    {
        Error err;
        const Vector<uint8_t>& source = FileAccess::get_file_as_bytes(p_path, &err);
        if (err != OK)
        {
            return err;
        }
        const CharString& cs_source = (const char*) source.ptr();
        const String& cs_path = p_path;

        return context_->eval(cs_source, cs_path);
    }

    bool JavaScriptBridgeLanguage::validate(const String& p_script, const String& p_path, List<String>* r_functions, List<ScriptError>* r_errors, List<Warning>* r_warnings, HashSet<int>* r_safe_lines) const
    {
        //TODO
        return false;
    }

    Ref<Script> JavaScriptBridgeLanguage::make_template(const String& p_template, const String& p_class_name, const String& p_base_class_name) const
    {
        //TODO
        return {};
    }

    Vector<ScriptLanguage::ScriptTemplate> JavaScriptBridgeLanguage::get_built_in_templates(StringName p_object)
    {
        //TODO
        return {};
    }

    void JavaScriptBridgeLanguage::reload_all_scripts()
    {
        //TODO
    }

    void JavaScriptBridgeLanguage::get_recognized_extensions(List<String>* p_extensions) const
    {
        //TODO
    }

    void JavaScriptBridgeLanguage::reload_script(const Ref<Script>& p_script, bool p_soft_reload)
    {
        //TODO
    }

}
