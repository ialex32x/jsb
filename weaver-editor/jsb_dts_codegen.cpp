#include "jsb_dts_codegen.h"
#include "core/core_bind.h"

namespace jsb::editor
{
    struct ModuleScope
    {

    };

    void TypeDeclarationCodeGen::emit()
    {
        Ref<FileAccess> out = FileAccess::open("./typescripts/typings/godot.autogen.d.ts", FileAccess::WRITE);

        out->store_line("declare module 'godot' {");
        for (const KeyValue<StringName, ClassDB::ClassInfo>& it : ClassDB::classes)
        {
            const ClassDB::ClassInfo& class_info = it.value;
            out->store_line(vformat("    class %s {", (String) class_info.name));
            for (const KeyValue<StringName, MethodBind*>& pair : class_info.method_map)
            {
                const StringName& method_name = pair.key;
                MethodBind* method_bind = pair.value;

                out->store_line(vformat("        %s(): any;", (String) method_name));
            }
            out->store_line("    }");
        }
        out->store_line("} // end of module 'godot'");
    }
}