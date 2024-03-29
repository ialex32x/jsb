#include "register_types.h"

#include "weaver/jsb_gdjs_lang.h"
#include "weaver/jsb_resource_loader.h"
#include "weaver/jsb_resource_saver.h"

Ref<ResourceFormatLoaderGodotJSScript> resource_loader_js;
Ref<ResourceFormatSaverGodotJSScript> resource_saver_js;

// #include "modules/regex/regex.h"
// #include "internal/jsb_source_map.h"
// static void test_regex(String str)
// {
//     Ref<RegEx> regex = str.contains("(") && str.contains(")")
//         ? RegEx::create_from_string("\\s+at\\s(.+)\\s\\((.+\\.js):(\\d+):(\\d+)\\)")
//         : RegEx::create_from_string("\\s+at\\s(.+\\.js):(\\d+):(\\d+)");
//     TypedArray<RegExMatch> matches = regex->search_all(str);
//
//     if (matches.size() == 1)
//     {
//         Ref<RegExMatch> match = (Ref<RegExMatch>)matches[0];
//         int group_index = match->get_group_count() - 3;
//         String name = group_index == 0 ? String() : match->get_string(group_index);
//         String filename = match->get_string(group_index + 1);
//         int64_t line = match->get_string(group_index+2).to_int();
//         int64_t col = match->get_string(group_index+3).to_int();
//         JSB_LOG(Verbose, "match %s %d,%d", filename, line, col);
//     }
// }
void initialize_jsb_module(ModuleInitializationLevel p_level)
{
    // test_regex("    at new DeleteMe (D:\\Projects\\godot\\modules\\jsb\\scripts\\out\\scratchpad.js:7:17)");
    // test_regex("    at D:\\Projects\\godot\\modules\\jsb\\scripts\\out\\main_entry.js:79:14");
    // JSB_LOG(Verbose, "regex test");
    // jsb::internal::SourceMap source_map;
    // constexpr char mappings[] = ";;;AAAA,iCAA6B;AAC7B,MAAa,QAAQ;CAAI;AAAzB,4BAAyB;AAAA,CAAC;AACb,QAAA,QAAQ,GAAG,CAAC,CAAC;AAC1B,MAAsB,aAAa;IAE/B,IAAI;QACA,OAAO,CAAC,GAAG,CAAC,kBAAkB,CAAC,CAAA;IACnC,CAAC;CACJ;AALD,sCAKC;AACD,MAAa,aAAc,SAAQ,aAAa;IAC5C,GAAG,KAAW,IAAI,CAAC,IAAI,EAAE,CAAC,CAAC,CAAC;CAC/B;AAFD,sCAEC;AACD,CAAC;IACG,MAAM,IAAI,GAAG,IAAI,YAAI,EAAE,CAAC;AAC5B,CAAC,CAAC,EAAE,CAAC;AACL,MAAM,SAAU,SAAQ,YAAI;CAAI;AAChC,MAAM,YAAa,SAAQ,SAAS;CAAI;AACxC,MAAqB,SAAU,SAAQ,YAAY;IAC/C;QACI,OAAO,CAAC,GAAG,CAAC,gCAAgC,CAAC,CAAC;QAC9C,KAAK,EAAE,CAAC;QACR,OAAO,CAAC,GAAG,CAAC,+BAA+B,CAAC,CAAC;IACjD,CAAC;CACJ;AAND,4BAMC";
    // source_map.parse_mappings(mappings, std::size(mappings) - 1);
    // jsb::internal::SourcePosition pos;
    // if (source_map.find(28, 8, pos))
    // {
    //     JSB_LOG(Verbose, "js(28, 8) => ts(%d, %d)", pos.line, pos.column);
    // }

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
