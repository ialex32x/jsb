
import { FileAccess } from "godot";

interface CodeWriter {
    line(text: string): this;
}

class IndentWriter implements CodeWriter {
    private _base: CodeWriter;

    constructor(base: CodeWriter) {
        this._base = base;
    }

    line(text: string): this {
        this._base.line("    " + text);
        return this;
    }
}

// d.ts generator
export default class TSDCodeGen implements CodeWriter {
    // private _filePath: string;
    private _file: FileAccess;

    constructor(filePath: string) {
        // this._filePath = filePath;
        this._file = FileAccess.open(filePath, FileAccess.WRITE);
    }

    line(text: string): this {
        this._file.store_line(text);
        return this;
    }

    emit() {
        const classes = jsb.editor.get_classes();
        for (let cls of classes) {
            this.line(`[Class] ${cls.name} < ${cls.super}`);
            let class_scope = new IndentWriter(this);

            for (let constant of cls.constants) {
                class_scope.line(`[Constant] ${constant.name} = ${constant.value}`);
            }
            for (let enum_info of cls.enums) {
                let values = enum_info.literals.join(", ");
                class_scope.line(`[Enum] ${enum_info.name} { ${values} }`);
            }
            for (let method_info of cls.methods) {
                class_scope.line(`[Method] ${method_info.name} ${method_info.argument_count}`);
            }
            for (let field_info of cls.fields) {
                class_scope.line(`[Field] ${field_info.name}: ${field_info.class_name} (${field_info.type})`);
            }
            for (let property_info of cls.properties) {
                class_scope.line(`[Property] ${property_info.name}: ${property_info.type} (${property_info.setter}, ${property_info.getter})`);
            }
            for (let signal_info of cls.signals) {
                class_scope.line(`[Signal] ${signal_info.name}: ${signal_info.name_} ${signal_info.flags}`);
            }
        }

        const singletons = jsb.editor.get_singletons();
        for (let singleton of singletons) {
            const user_created = singleton.user_created ? "(USER)" : "";
            const editor_only = singleton.editor_only ? "(EDITOR)" : "";
            this.line(`[Singleton] ${singleton.name}: ${singleton.class_name} ${user_created}${editor_only}`);
        }

        const global_constants = jsb.editor.get_global_constants();
        for (let global_constant of global_constants) {
            this.line(`[GlobalConstant] ${global_constant.name} = ${global_constant.value}`);
        }

        this._file.flush();
    }
}

