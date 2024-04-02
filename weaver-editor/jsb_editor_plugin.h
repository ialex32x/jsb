#ifndef JAVASCRIPT_EDITOR_PLUGINS_H
#define JAVASCRIPT_EDITOR_PLUGINS_H

#include "jsb_editor_macros.h"
#include "editor/editor_plugin.h"

class GodotJSEditorPlugin : public EditorPlugin
{
    GDCLASS(GodotJSEditorPlugin, EditorPlugin)

private:

protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _on_menu_pressed(int p_what);

public:
    GodotJSEditorPlugin();
    virtual ~GodotJSEditorPlugin() override;
};

#endif
