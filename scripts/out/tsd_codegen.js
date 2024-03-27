"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const godot_1 = require("godot");
const PrimitiveTypes = {
    [jsb.editor.Type.NIL]: "undefined",
    [jsb.editor.Type.BOOL]: "boolean",
    [jsb.editor.Type.INT]: "number /*i64*/",
    [jsb.editor.Type.FLOAT]: "number /*f64*/",
    [jsb.editor.Type.STRING]: "string",
    [jsb.editor.Type.VECTOR2]: "any /*Vector2*/",
    //TODO
    //...
};
class IndentWriter {
    constructor(base) {
        this._base = base;
        this._lines = [];
    }
    finish() {
        for (var line of this._lines) {
            this._base.line("    " + line);
        }
    }
    line_comment(text) {
        this.line(`// ${text}`);
        return this;
    }
    line(text) {
        this._lines.push(text);
        return this;
    }
}
class ModuleWriter extends IndentWriter {
    constructor(base, name) {
        super(base);
        this._name = name;
    }
    finish() {
        if (/^[a-zA-Z]+$/.test(this._name)) {
            this._base.line(`declare module ${this._name} {`);
        }
        else {
            this._base.line(`declare module "${this._name}" {`);
        }
        super.finish();
        this._base.line('}');
    }
    namespace_(name) {
        return new NamespaceWriter(this, name);
    }
    class_(name, super_) {
        return new ClassWriter(this, name, super_);
    }
    singleton_(info) {
        if (info.editor_only) {
            this.line_comment(`@editor`);
        }
        if (info.class_name.length != 0) {
            this.line(`const ${info.name}: godot.${info.class_name}`);
        }
        else {
            this.line(`const ${info.name}: any /*unknown*/`);
        }
    }
}
class NamespaceWriter extends IndentWriter {
    constructor(base, name) {
        super(base);
        this._name = name;
    }
    finish() {
        if (this._lines.length == 0) {
            return;
        }
        this._base.line(`namespace ${this._name} {`);
        super.finish();
        this._base.line('}');
    }
    enum_(name) {
        return new EnumWriter(this, name);
    }
}
class ClassWriter extends IndentWriter {
    constructor(base, name, super_) {
        super(base);
        this._name = name;
        this._super = super_;
    }
    finish() {
        if (typeof this._super !== "string" || this._super.length == 0) {
            this._base.line(`class ${this._name} {`);
        }
        else {
            this._base.line(`class ${this._name} extends ${this._super} {`);
        }
        super.finish();
        this._base.line('}');
    }
    constant_(constant) {
        this.line(`static readonly ${constant.name} = ${constant.value}`);
    }
    make_typename(info) {
        if (info.class_name.length == 0) {
            const primitive_name = PrimitiveTypes[info.type];
            if (typeof primitive_name !== "undefined") {
                return primitive_name;
            }
            return `any /*unhandled: ${info.type}*/`;
        }
        return info.class_name;
    }
    make_arg(info) {
        return `${info.name}: ${this.make_typename(info)}`;
    }
    make_args(method_info) {
        //TODO
        if (method_info.args_.length == 0) {
            return "";
        }
        return method_info.args_.map(it => this.make_arg(it)).join(",");
    }
    make_return(method_info) {
        //TODO
        if (typeof method_info.return_ != "undefined") {
            return this.make_typename(method_info.return_);
        }
        return "void";
    }
    method_(method_info) {
        const args = this.make_args(method_info);
        const rval = this.make_return(method_info);
        if (method_info.is_static) {
            this.line(`static ${method_info.name}(${args}): ${rval}`);
        }
        else {
            this.line(`${method_info.name}(${args}): ${rval}`);
        }
    }
    signal_(signal_info) {
        this.line_comment(`SIGNAL: ${signal_info.name}`);
    }
}
class EnumWriter extends IndentWriter {
    constructor(base, name) {
        super(base);
        this._name = name;
    }
    finish() {
        if (this._lines.length == 0) {
            return;
        }
        this._base.line(`enum ${this._name} {`);
        super.finish();
        this._base.line('}');
    }
    element_(name, value) {
        this.line(`${name} = ${value},`);
    }
}
// d.ts generator
class TSDCodeGen {
    constructor(filePath) {
        // this._filePath = filePath;
        this._file = godot_1.FileAccess.open(filePath, godot_1.FileAccess.WRITE);
        this._singletons = {};
        this._classes = {};
        this.line("// AUTO-GENERATED");
        const classes = jsb.editor.get_classes();
        const singletons = jsb.editor.get_singletons();
        for (let cls of classes) {
            this._classes[cls.name] = cls;
        }
        for (let singleton of singletons) {
            this._singletons[singleton.name] = singleton;
        }
    }
    finish() {
        this._file.flush();
    }
    line(text) {
        this._file.store_line(text);
        return this;
    }
    has_class(name) {
        return typeof this._classes[name] !== "undefined";
    }
    emit() {
        this.emit_singletons();
        this.emit_godot();
    }
    emit_singletons() {
        const module_cg = new ModuleWriter(this, "godot-globals");
        for (let singleton_name in this._singletons) {
            const singleton = this._singletons[singleton_name];
            module_cg.singleton_(singleton);
        }
        module_cg.finish();
    }
    emit_godot() {
        const module_cg = new ModuleWriter(this, "godot");
        for (let class_name in this._classes) {
            const cls = this._classes[class_name];
            const ignored_consts = new Set();
            const class_ns_cg = module_cg.namespace_(cls.name);
            for (let enum_info of cls.enums) {
                const enum_cg = class_ns_cg.enum_(enum_info.name);
                for (let name of enum_info.literals) {
                    const value = cls.constants.find(v => v.name == name).value;
                    enum_cg.element_(name, value);
                    ignored_consts.add(name);
                }
                enum_cg.finish();
            }
            class_ns_cg.finish();
            const class_cg = module_cg.class_(cls.name, this.has_class(cls.super) ? cls.super : "");
            for (let constant of cls.constants) {
                if (!ignored_consts.has(constant.name)) {
                    class_cg.constant_(constant);
                }
            }
            for (let method_info of cls.methods) {
                class_cg.method_(method_info);
            }
            // for (let field_info of cls.fields) {
            //     class_scope.line(`[Field] ${field_info.name}: ${field_info.class_name} (${field_info.type})`);
            // }
            // for (let property_info of cls.properties) {
            //     class_scope.line(`[Property] ${property_info.name}: ${property_info.type} (${property_info.setter}, ${property_info.getter})`);
            // }
            for (let signal_info of cls.signals) {
                class_cg.signal_(signal_info);
            }
            class_cg.finish();
        }
        // const global_constants = jsb.editor.get_global_constants();
        // for (let global_constant of global_constants) {
        //     this.line(`[GlobalConstant] ${global_constant.name} = ${global_constant.value}`);
        // }
        module_cg.finish();
    }
}
exports.default = TSDCodeGen;
//# sourceMappingURL=tsd_codegen.js.map