#include "jsb_editor_plugin.h"

#include "jsb_project_preset.h"
#include "jsb_repl.h"
#include "../internal/jsb_path_util.h"
#include "../weaver/jsb_gdjs_lang.h"

#include "core/config/project_settings.h"
#include "scene/gui/popup_menu.h"
#include "editor/editor_settings.h"
#include "editor/gui/editor_toaster.h"

enum
{
    MENU_ID_INSTALL_TS_PROJECT,
    MENU_ID_GENERATE_GODOT_DTS,
};

void GodotJSEditorPlugin::_bind_methods()
{

}

void GodotJSEditorPlugin::_notification(int p_what)
{
    // switch (p_what)
    // {
    // case NOTIFICATION_APPLICATION_FOCUS_IN: break;
    // default: break;
    // }
}

void GodotJSEditorPlugin::_on_menu_pressed(int p_what)
{
    switch (p_what)
    {
    case MENU_ID_INSTALL_TS_PROJECT: install_ts_project(); break;
    case MENU_ID_GENERATE_GODOT_DTS: generate_godot_dts(); break;
    default: break;
    }
}

GodotJSEditorPlugin::GodotJSEditorPlugin()
{
    PopupMenu *menu = memnew(PopupMenu);
    add_tool_submenu_item(TTR("GodotJS"), menu);
    menu->add_item(TTR("Install TS Project"), MENU_ID_INSTALL_TS_PROJECT);
    menu->add_item(TTR("Generate Godot d.ts"), MENU_ID_GENERATE_GODOT_DTS);
    menu->connect("id_pressed", callable_mp(this, &GodotJSEditorPlugin::_on_menu_pressed));

    add_control_to_bottom_panel(memnew(GodotJSREPL), TTR("GodotJS"));
}

GodotJSEditorPlugin::~GodotJSEditorPlugin()
{
    JSB_LOG(Verbose, "~GodotJSEditorPlugin");
}

Error GodotJSEditorPlugin::write_file(const String &p_target_dir, const String &p_source_name)
{
    Error err;
    size_t size;
    const char* data = GodotJSPorjectPreset::get_source(p_source_name, size);
    ERR_FAIL_COND_V_MSG(size == 0 || data == nullptr, ERR_FILE_NOT_FOUND, "bad data");
    err = DirAccess::make_dir_recursive_absolute(p_target_dir);
    ERR_FAIL_COND_V_MSG(err != OK, err, "failed to make dir");
    const String target_name = jsb::internal::PathUtil::combine(p_target_dir, p_source_name);
    const Ref<FileAccess> outfile = FileAccess::open(target_name, FileAccess::WRITE, &err);
    ERR_FAIL_COND_V_MSG(err != OK, err, "failed to open output file");
    outfile->store_buffer((const uint8_t*) data, size);
    return OK;
}

void GodotJSEditorPlugin::install_ts_project()
{
    // avoid overwriting existed ts project files
    ERR_FAIL_COND_MSG(FileAccess::exists("res://typescripts/tsconfig.json"), "found an existed tsconfig.json, please delete it before installing the TS project presets.");

    // config files
    ERR_FAIL_COND(write_file("res://typescripts", "tsconfig.json") != OK);
    ERR_FAIL_COND(write_file("res://typescripts", "package.json") != OK);
    ERR_FAIL_COND(write_file("res://typescripts", ".gdignore") != OK);

    // type declaration files
    ERR_FAIL_COND(write_file("res://typescripts/typings", "godot.minimal.d.ts") != OK);

    // ts source files
    ERR_FAIL_COND(write_file("res://typescripts/src/jsb", "jsb.core.ts") != OK);
    ERR_FAIL_COND(write_file("res://typescripts/src/jsb", "jsb.editor.codegen.ts") != OK);
    ERR_FAIL_COND(write_file("res://typescripts/src/jsb", "jsb.editor.main.ts") != OK);

    // optional files which could be generated from ts source with tsc by the user
    ERR_FAIL_COND(write_file("res://javascripts/jsb", "jsb.core.js") != OK);
    ERR_FAIL_COND(write_file("res://javascripts/jsb", "jsb.editor.codegen.js") != OK);
    ERR_FAIL_COND(write_file("res://javascripts/jsb", "jsb.editor.main.js") != OK);

    write_file("res://javascripts/jsb", "jsb.core.js.map");
    write_file("res://javascripts/jsb", "jsb.editor.codegen.js.map");
    write_file("res://javascripts/jsb", "jsb.editor.main.js.map");
    write_file("res://javascripts/icon", "filetype-js.svg");

    generate_godot_dts();
    load_editor_entry_module();

    const String toast_message = TTR("TS project installed, place your ts code in res://typescripts and compile with tsc command under `typescripts` directory.");
    EditorToaster::get_singleton()->popup_str(toast_message, EditorToaster::SEVERITY_INFO);
}

void GodotJSEditorPlugin::generate_godot_dts()
{
    ERR_FAIL_COND_MSG(!FileAccess::exists("res://javascripts/jsb/jsb.editor.codegen.js"), "install and compile ts source at first");

    GodotJSScriptLanguage* lang = GodotJSScriptLanguage::get_singleton();
    jsb_check(lang);
    const Error err = lang->eval_source(R"--((function(){const mod = require("jsb/jsb.editor.codegen"); (new mod.default("./typescripts/typings")).emit();})())--");
    ERR_FAIL_COND_MSG(err != OK, "failed to evaluate jsb.editor.codegen");
}

void GodotJSEditorPlugin::load_editor_entry_module()
{
    ERR_FAIL_COND_MSG(!FileAccess::exists("res://javascripts/jsb/jsb.editor.main.js"), "install and compile ts source at first");

    GodotJSScriptLanguage* lang = GodotJSScriptLanguage::get_singleton();
    jsb_check(lang);
    const Error err = lang->get_context()->load("jsb/jsb.editor.main");
    ERR_FAIL_COND_MSG(err != OK, "failed to evaluate jsb.editor.codegen");
}
