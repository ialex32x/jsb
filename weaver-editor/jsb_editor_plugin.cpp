#include "jsb_editor_plugin.h"

#include "core/config/project_settings.h"
#include "editor/editor_settings.h"
#include "scene/gui/popup_menu.h"

enum
{
    MENU_ID_INSTALL_TS_PROJECT,
};

void GodotJSEditorPlugin::_bind_methods()
{

}

void GodotJSEditorPlugin::_notification(int p_what)
{

}

void GodotJSEditorPlugin::_on_menu_pressed(int p_what)
{
    switch (p_what)
    {
    case MENU_ID_INSTALL_TS_PROJECT: JSB_LOG(Verbose, "test"); break;
    default: break;
    }
}

GodotJSEditorPlugin::GodotJSEditorPlugin()
{
    PopupMenu *menu = memnew(PopupMenu);
    add_tool_submenu_item(TTR("GodotJS"), menu);
    menu->add_item(TTR("Install TS Project"), MENU_ID_INSTALL_TS_PROJECT);
    menu->connect("id_pressed", callable_mp(this, &GodotJSEditorPlugin::_on_menu_pressed));
}

GodotJSEditorPlugin::~GodotJSEditorPlugin()
{
    JSB_LOG(Verbose, "~GodotJSEditorPlugin");
}
