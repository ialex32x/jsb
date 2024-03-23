#include "jsb_resource_loader.h"

#include "jsb_lang.h"
#include "jsb_script.h"
#include "jsb_weaver_consts.h"

Ref<Resource> ResourceFormatLoaderJavaScript::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode)
{
    //TODO use JavaScriptContext to resolve?
    Error err;
    jsb_check(p_path.ends_with(JSB_RES_EXT));
    const String code = FileAccess::get_file_as_string(p_path, &err);
    if (err != OK)
    {
        if (r_error) *r_error = err;
        return {};
    }
    Ref<JavaScript> spt;
    spt.instantiate();
    spt->set_path(p_path);
    spt->set_source_code(code);
    return spt;
}

void ResourceFormatLoaderJavaScript::get_recognized_extensions(List<String> *p_extensions) const
{
    p_extensions->push_back(JSB_RES_EXT);
}

bool ResourceFormatLoaderJavaScript::handles_type(const String &p_type) const
{
	return (p_type == "Script" || p_type == JSB_RES_TYPE);
}

String ResourceFormatLoaderJavaScript::get_resource_type(const String &p_path) const
{
    const String el = p_path.get_extension().to_lower();
    return el == JSB_RES_EXT ? JSB_RES_TYPE : "";
}

void ResourceFormatLoaderJavaScript::get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types)
{
    //TODO
}
