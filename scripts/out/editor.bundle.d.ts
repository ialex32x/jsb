/// <reference path="../godot-typings/godot.generated.d.ts" />
declare module "tsd_codegen" {
    export default class TSDCodeGen {
        private _split_index;
        private _outDir;
        private _splitter;
        private _types;
        constructor(outDir: string);
        private make_path;
        private new_splitter;
        private split;
        private cleanup;
        has_class(name: string): boolean;
        emit(): void;
        private emit_mock;
        private emit_singletons;
        private emit_globals;
        private emit_godot;
        private emit_godot_class;
    }
}
declare module "scratchpad" {
    import { Node } from "godot";
    export class DeleteMe {
        constructor();
    }
    export const deleteMe = 1;
    export abstract class AbstractClass {
        abstract foo(): void;
        bark(): void;
    }
    export class ConcreteClass extends AbstractClass {
        foo(): void;
    }
    class LocalNode extends Node {
    }
    class SubLocalNode extends LocalNode {
    }
    export default class TestClass extends SubLocalNode {
        constructor();
    }
}
declare module "main_entry" { }
