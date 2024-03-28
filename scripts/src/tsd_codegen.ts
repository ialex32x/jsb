
import { FileAccess } from "godot";

interface CodeWriter {
    get size(): number;
    get lineno(): number;

    line(text: string): void;

    namespace_(name: string): NamespaceWriter;
    class_(name: string, super_: string): ClassWriter;
    singleton_(info: jsb.editor.SingletonInfo): SingletonWriter;
    line_comment_(text: string): void;
}

interface ScopeWriter extends CodeWriter {
    finish(): void;
}

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
}

abstract class AbstractWriter implements ScopeWriter {
    abstract line(text: string): void;
    abstract finish(): void;
    abstract get size(): number;
    abstract get lineno(): number;

    constructor() { }

    namespace_(name: string): NamespaceWriter {
        return new NamespaceWriter(this, name)
    }
    class_(name: string, super_: string): ClassWriter {
        return new ClassWriter(this, name, super_);
    }
    singleton_(info: jsb.editor.SingletonInfo): SingletonWriter {
        return new SingletonWriter(this, info);
    }
    line_comment_(text: string) {
        this.line(`// ${text}`);
    }
}

const tab = "    ";

class IndentWriter extends AbstractWriter {
    protected _base: ScopeWriter;
    protected _lines: string[];
    protected _size: number = 0;

    constructor(base: ScopeWriter) {
        super();
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

    line(text: string): void {
        this._lines.push(text);
        this._size += tab.length + text.length;
    }
}

class ModuleWriter extends IndentWriter {
    protected _name: string;

    constructor(base: ScopeWriter, name: string) {
        super(base);
        this._name = name;
    }

    finish() {
        if (/^[a-zA-Z]+$/.test(this._name)) {
            this._base.line(`declare module ${this._name} {`);
        } else {
            this._base.line(`declare module "${this._name}" {`);
        }
        super.finish();
        this._base.line('}');
    }
}

class NamespaceWriter extends IndentWriter {
    protected _name: string;

    constructor(base: ScopeWriter, name: string) {
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

    enum_(name: string): EnumWriter {
        return new EnumWriter(this, name);
    }
}

class ClassWriter extends IndentWriter {
    protected _name: string;
    protected _super: string;

    constructor(base: ScopeWriter, name: string, super_: string) {
        super(base);
        this._name = name;
        this._super = super_;
    }
    protected head() {
        if (typeof this._super !== "string" || this._super.length == 0) {
            return `class ${this._name}`
        }
        return `class ${this._name} extends ${this._super}`
    }
    finish() {
        this._base.line(`${this.head()} {`)
        super.finish()
        this._base.line('}')
    }
    constant_(constant: jsb.editor.ConstantInfo) {
        this.line(`static readonly ${constant.name} = ${constant.value}`);
    }
    private make_typename(info: jsb.editor.PropertyInfo): string {
        if (info.class_name.length == 0) {
            const primitive_name = PrimitiveTypes[info.type];
            if (typeof primitive_name !== "undefined") {
                return primitive_name
            }
            return `any /*unhandled: ${info.type}*/`
        }
        return info.class_name
    }
    private make_arg(info: jsb.editor.PropertyInfo): string {
        return `${info.name}: ${this.make_typename(info)}`
    }
    private make_args(method_info: jsb.editor.MethodInfo): string {
        //TODO
        if (method_info.args_.length == 0) {
            return ""
        }
        return method_info.args_.map(it => this.make_arg(it)).join(", ");
    }
    private make_return(method_info: jsb.editor.MethodInfo): string {
        //TODO
        if (typeof method_info.return_ != "undefined") {
            return this.make_typename(method_info.return_)
        }
        return "void"
    }
    method_(method_info: jsb.editor.MethodInfo) {
        const args = this.make_args(method_info)
        const rval = this.make_return(method_info)
        if (method_info.is_static) {
            this.line(`static ${method_info.name}(${args}): ${rval}`);
        } else {
            this.line(`${method_info.name}(${args}): ${rval}`);
        }
    }
    signal_(signal_info: jsb.editor.SignalInfo) {
        this.line_comment_(`SIGNAL: ${signal_info.name}`)
    }
}

class SingletonWriter extends ClassWriter {
    protected _info: jsb.editor.SingletonInfo
    constructor(base: ScopeWriter, info: jsb.editor.SingletonInfo) {
        super(base, info.name, "");
        this._info = info;
    }
    protected head() {
        return `const ${this._name} :`
    }
}

class EnumWriter extends IndentWriter {
    protected _name: string;

    constructor(base: ScopeWriter, name: string) {
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

    element_(name: string, value: number) {
        this.line(`${name} = ${value},`)
    }
}

class FileWriter extends AbstractWriter {
    private _file: FileAccess;
    private _size = 0;
    private _lineno = 0;

    constructor(file: FileAccess) {
        super();
        this._file = file;
    }

    get size() { return this._size; }
    get lineno() { return this._lineno; }

    line(text: string): void {
        this._file.store_line(text);
        this._size += text.length;
        this._lineno += 1;
    }

    finish(): void {
        this._file.flush();
    }
}

class FileSplitter {
    private _file: FileAccess;
    private _toplevel: ScopeWriter;

    constructor(filePath: string) {
        this._file = FileAccess.open(filePath, FileAccess.WRITE);
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
export default class TSDCodeGen {
    private _split_index: number;
    private _outDir: string;
    private _splitter: FileSplitter | undefined;
    private _singletons: { [name: string]: jsb.editor.SingletonInfo };
    private _classes: { [name: string]: jsb.editor.ClassInfo };

    constructor(outDir: string) {
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

    private make_path(index: number) {
        const filename = `godot${index}.gen.d.ts`;
        if (typeof this._outDir !== "string" || this._outDir.length == 0) {
            return filename;
        }
        if (this._outDir.endsWith("/")) {
            return this._outDir + filename;
        }
        return this._outDir + "/" + filename;
    }

    private new_splitter() {
        if (this._splitter !== undefined) {
            this._splitter.close();
        }
        const filename = this.make_path(this._split_index++);
        console.log("new writer", filename);
        this._splitter = new FileSplitter(filename);
        return this._splitter;
    }

    // the returned writer will be `finished` automatically
    private split() : CodeWriter {
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

    private cleanup() {
        while (true) {
            const path = this.make_path(this._split_index++);
            if (!FileAccess.file_exists(path)) {
                break;
            }
            console.warn("delete file", path);
            jsb.editor.delete_file(path);
        }
    }

    has_class(name: string): boolean {
        return typeof this._classes[name] !== "undefined"
    }

    emit() {
        this.emit_singletons();
        this.emit_godot();
        this._splitter?.close();
        this.cleanup();
    }

    private emit_singletons() {
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
            } else {
                cg.line_comment_(`ERROR: singleton ${singleton.name} without class info ${singleton.class_name}`)
            }
        }
    }

    private emit_godot() {
        for (let class_name in this._classes) {
            const cls = this._classes[class_name];
            const cg = this.split();

            const ignored_consts: Set<string> = new Set();
            const class_ns_cg = cg.namespace_(cls.name);
            for (let enum_info of cls.enums) {
                const enum_cg = class_ns_cg.enum_(enum_info.name);
                for (let name of enum_info.literals) {
                    const value = cls.constants.find(v => v.name == name)!.value;
                    enum_cg.element_(name, value)
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

