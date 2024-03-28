"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const godot_1 = require("godot");
const PrimitiveTypes = {
    [jsb.editor.Type.NIL]: "undefined",
    [jsb.editor.Type.BOOL]: "boolean",
    [jsb.editor.Type.INT]: "number /*i64*/",
    [jsb.editor.Type.FLOAT]: "number /*f64*/",
    [jsb.editor.Type.STRING]: "string",
    //TODO
    //...
    [jsb.editor.Type.DICTIONARY]: "GodotDictionary",
    [jsb.editor.Type.ARRAY]: "GodotArray",
};
class AbstractWriter {
    constructor() { }
    namespace_(name) {
        return new NamespaceWriter(this, name);
    }
    class_(name, super_) {
        return new ClassWriter(this, name, super_);
    }
    singleton_(info) {
        return new SingletonWriter(this, info);
    }
    line_comment_(text) {
        this.line(`// ${text}`);
    }
}
const tab = "    ";
class IndentWriter extends AbstractWriter {
    constructor(base) {
        super();
        this._size = 0;
        this._base = base;
        this._lines = [];
    }
    get size() { return this._size; }
    get lineno() { return this._lines.length; }
    finish() {
        for (var line of this._lines) {
            this._base.line(tab + line);
        }
    }
    line(text) {
        this._lines.push(text);
        this._size += tab.length + text.length;
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
    head() {
        if (typeof this._super !== "string" || this._super.length == 0) {
            return `class ${this._name}`;
        }
        return `class ${this._name} extends ${this._super}`;
    }
    finish() {
        this._base.line(`${this.head()} {`);
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
        return method_info.args_.map(it => this.make_arg(it)).join(", ");
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
        this.line_comment_(`SIGNAL: ${signal_info.name}`);
    }
}
class SingletonWriter extends ClassWriter {
    constructor(base, info) {
        super(base, info.name, "");
        this._info = info;
    }
    head() {
        return `const ${this._name} :`;
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
class FileWriter extends AbstractWriter {
    constructor(file) {
        super();
        this._size = 0;
        this._lineno = 0;
        this._file = file;
    }
    get size() { return this._size; }
    get lineno() { return this._lineno; }
    line(text) {
        this._file.store_line(text);
        this._size += text.length;
        this._lineno += 1;
    }
    finish() {
        this._file.flush();
    }
}
class FileSplitter {
    constructor(filePath) {
        this._file = godot_1.FileAccess.open(filePath, godot_1.FileAccess.WRITE);
        this._toplevel = new ModuleWriter(new FileWriter(this._file), "godot");
        this._file.store_line("// AUTO-GENERATED");
        this._file.store_line('/// <reference no-default-lib="true"/>');
    }
    close() {
        this._toplevel.finish();
        this._file.flush();
        this._file.close();
    }
    get_writer() {
        return this._toplevel;
    }
    get_size() { return this._toplevel.size; }
    get_lineno() { return this._toplevel.lineno; }
}
// d.ts generator
class TSDCodeGen {
    constructor(outDir) {
        this._split_index = 0;
        this._outDir = outDir;
        this._singletons = {};
        this._classes = {};
        const classes = jsb.editor.get_classes();
        const singletons = jsb.editor.get_singletons();
        for (let cls of classes) {
            this._classes[cls.name] = cls;
        }
        for (let singleton of singletons) {
            this._singletons[singleton.name] = singleton;
        }
    }
    make_path(index) {
        const filename = `godot${index}.gen.d.ts`;
        if (typeof this._outDir !== "string" || this._outDir.length == 0) {
            return filename;
        }
        if (this._outDir.endsWith("/")) {
            return this._outDir + filename;
        }
        return this._outDir + "/" + filename;
    }
    new_splitter() {
        if (this._splitter !== undefined) {
            this._splitter.close();
        }
        const filename = this.make_path(this._split_index++);
        console.log("new writer", filename);
        this._splitter = new FileSplitter(filename);
        return this._splitter;
    }
    // the returned writer will be `finished` automatically
    split() {
        if (this._splitter == undefined) {
            return this.new_splitter().get_writer();
        }
        const len = this._splitter.get_size();
        const lineno = this._splitter.get_lineno();
        if (len > 1024 * 900 || lineno > 12000) {
            return this.new_splitter().get_writer();
        }
        return this._splitter.get_writer();
    }
    cleanup() {
        while (true) {
            const path = this.make_path(this._split_index++);
            if (!godot_1.FileAccess.file_exists(path)) {
                break;
            }
            console.warn("delete file", path);
            jsb.editor.delete_file(path);
        }
    }
    has_class(name) {
        return typeof this._classes[name] !== "undefined";
    }
    emit() {
        var _a;
        this.emit_singletons();
        this.emit_godot();
        (_a = this._splitter) === null || _a === void 0 ? void 0 : _a.close();
        this.cleanup();
    }
    emit_singletons() {
        const cg = this.split();
        for (let singleton_name in this._singletons) {
            const singleton = this._singletons[singleton_name];
            const cls = this._classes[singleton.class_name];
            if (typeof cls !== "undefined") {
                const class_cg = cg.singleton_(singleton);
                // for (let constant of cls.constants) {
                //     if (!ignored_consts.has(constant.name)) {
                //         class_cg.constant_(constant);
                //     }
                // }
                for (let method_info of cls.methods) {
                    class_cg.method_(method_info);
                }
                class_cg.finish();
            }
            else {
                cg.line_comment_(`ERROR: singleton ${singleton.name} without class info ${singleton.class_name}`);
            }
        }
    }
    emit_godot() {
        for (let class_name in this._classes) {
            const cls = this._classes[class_name];
            const cg = this.split();
            const ignored_consts = new Set();
            const class_ns_cg = cg.namespace_(cls.name);
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
            const class_cg = cg.class_(cls.name, this.has_class(cls.super) ? cls.super : "");
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
    }
}
exports.default = TSDCodeGen;
//# sourceMappingURL=tsd_codegen.js.map