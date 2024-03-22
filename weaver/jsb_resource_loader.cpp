#include "jsb_resource_loader.h"

Ref<Resource> ResourceFormatLoaderJavaScript::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode)
{
    //TODO
    return {};
}

void ResourceFormatLoaderJavaScript::get_recognized_extensions(List<String> *p_extensions) const
{
    //TODO
}

bool ResourceFormatLoaderJavaScript::handles_type(const String &p_type) const
{
    //TODO
    return false;
}

String ResourceFormatLoaderJavaScript::get_resource_type(const String &p_path) const
{
    String el = p_path.get_extension().to_lower();
    if (el == "js") {
        return "JavaScript";
    }
    return "";
}

void ResourceFormatLoaderJavaScript::get_dependencies(const String &p_path, List<String> *p_dependencies, bool p_add_types)
{
    //TODO
}

