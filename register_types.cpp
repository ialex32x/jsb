#include "register_types.h"

#include "weaver/jsb_lang.h"

#define JSB_LANG_ENABLED 0

#if JSB_LANG_ENABLED
static jsb::JavaScriptLanguage* script_language_js = nullptr;
#endif

void initialize_jsb_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
    {
        //TODO simple dirty temp code for testing
        {
            std::shared_ptr<jsb::JavaScriptRuntime> cruntime = std::make_shared<jsb::JavaScriptRuntime>();
            std::shared_ptr<jsb::JavaScriptContext> ccontext = std::make_shared<jsb::JavaScriptContext>(cruntime);

            {
                //TODO temp code
                ccontext->expose();
            }

            // not all classes available in the LEVEL_CORE phase
            static CharString source =
                "print('hello, v8!');\n"
                "let godot = require('godot');\n"
                "let RefCounted = godot.RefCounted;\n"
                "print(RefCounted);\n"
                "print(new RefCounted());\n"
                "{ let f = new Foo(); print(f.test(1122)); }\n";
            ccontext->eval(source, "eval-1");
            ccontext->eval(
                "print('right');\n"
                "something wrong\n", "eval-2");
            // rt->gc();
        }
        print_line("jsb temp code ok");

        // register javascript language
#if JSB_LANG_ENABLED
        script_language_js = memnew(jsb::JavaScriptLanguage());
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
