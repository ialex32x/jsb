#include "jsb_resource_saver.h"
#include "jsb_script.h"
#include "core/error/error_list.h"

Error ResourceFormatSaverJavaScript::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
    Ref<JavaScript> sqscr = p_resource;
    ERR_FAIL_COND_V(sqscr.is_null(), ERR_INVALID_PARAMETER);

    String source = sqscr->get_source_code();

    {
        Error err;
        Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

        ERR_FAIL_COND_V_MSG(err, err, "Cannot save JavaScript file '" + p_path + "'.");

        file->store_string(source);
        if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
            return ERR_CANT_CREATE;
        }
    }

    if (ScriptServer::is_reload_scripts_on_save_enabled()) {
        //TODO !!!
        //JavaScriptLanguage::get_singleton()->reload_tool_script(p_resource, true);
    }

    return OK;
}

void ResourceFormatSaverJavaScript::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
    if (Object::cast_to<JavaScript>(*p_resource)) {
        p_extensions->push_back("js");
    }
}

bool ResourceFormatSaverJavaScript::recognize(const Ref<Resource> &p_resource) const {
    return Object::cast_to<JavaScript>(*p_resource) != nullptr;
}
