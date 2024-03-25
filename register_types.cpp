#include "register_types.h"

#include "weaver/jsb_gdjs_lang.h"
#include "weaver/jsb_resource_loader.h"
#include "weaver/jsb_resource_saver.h"

Ref<ResourceFormatLoaderGodotJSScript> resource_loader_js;
Ref<ResourceFormatSaverGodotJSScript> resource_saver_js;

void initialize_jsb_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
    {
        // register javascript language
        GodotJSScriptLanguage* script_language_js = memnew(GodotJSScriptLanguage());
	    ScriptServer::register_language(script_language_js);

		resource_loader_js.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_js);

		resource_saver_js.instantiate();
		ResourceSaver::add_resource_format_saver(resource_saver_js);

    }
}

void uninitialize_jsb_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_CORE)
    {
		ResourceLoader::remove_resource_format_loader(resource_loader_js);
		resource_loader_js.unref();

		ResourceSaver::remove_resource_format_saver(resource_saver_js);
		resource_saver_js.unref();

        GodotJSScriptLanguage *script_language_js = GodotJSScriptLanguage::get_singleton();
        jsb_check(script_language_js);
        ScriptServer::unregister_language(script_language_js);
        memdelete(script_language_js);
    }
}
