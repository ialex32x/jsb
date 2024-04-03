#ifndef JAVASCRIPT_PROJECT_PRESET_H
#define JAVASCRIPT_PROJECT_PRESET_H

#include "jsb_editor_macros.h"
#include "core/string/ustring.h"

struct GodotJSPorjectPreset
{
    static String get_source(const String &p_filename);
};
#endif