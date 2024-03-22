#include "register_types.h"

#include "internal/jsb_path_util.h"
#include "weaver/jsb_lang.h"

#define JSB_LANG_ENABLED 1

#if JSB_LANG_ENABLED
static JavaScriptLanguage* script_language_js = nullptr;
#endif

void initialize_jsb_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
    {
        // //TODO simple dirty temp code for testing
        // {
        //     std::shared_ptr<jsb::JavaScriptRuntime> cruntime = std::make_shared<jsb::JavaScriptRuntime>();
        //     std::shared_ptr<jsb::JavaScriptContext> ccontext = std::make_shared<jsb::JavaScriptContext>(cruntime);
        //
        //     cruntime->add_module_resolver<jsb::DefaultModuleResolver>();
        //     cruntime->add_module_resolver<jsb::FileSystemModuleResolver>()
        //         .add_search_path(jsb::internal::PathUtil::combine(
        //         jsb::internal::PathUtil::dirname(::OS::get_singleton()->get_executable_path()),
        //         "../modules/jsb/weaver-editor/scripts"));
        //
        //     {
        //         //TODO temp code
        //         ccontext->expose();
        //     }
        //
        //     ccontext->load("main_entry");
        //     // not all classes available in the LEVEL_CORE phase
        //     static CharString source =
        //         "{ let f = new Foo(); console.log(f.test(1122)); }\n"
        //         "require('javascripts/main');\n"
        //         "";
        //     ccontext->eval(source, "eval-1");
        //     ccontext->eval("console.log('right'); something wrong prevents compilation\n", "eval-2");
        //     // rt->gc();
        // }
        print_line("jsb temp code ok");

        // register javascript language
#if JSB_LANG_ENABLED
        script_language_js = memnew(JavaScriptLanguage());
	    ScriptServer::register_language(script_language_js);
#endif
    }
}

void uninitialize_jsb_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
    {
#if JSB_LANG_ENABLED
        ScriptServer::unregister_language(script_language_js);
        memdelete(script_language_js);
#endif
    }
}
