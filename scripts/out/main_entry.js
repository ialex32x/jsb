"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
// godot types will be exposed to JS runtime until they are actually used
const godot_1 = require("godot");
const tsd_codegen_1 = require("./tsd_codegen");
// entry point (editor only)
// log, warn, error, assert are essentially supported
console.log("main entry in editor");
console.assert(typeof godot_1.Node === "function");
let timer_id;
let times = 5;
// setInterval, setTimeout, setImmediate, clearInterval, clearTimeout are essentially supported
// NOTE: they return a timer_id which is an integer instead of Timeout object like in NodeJS
timer_id = setInterval(function () {
    console.log("tick");
    // for (let i = 0; i < 50; ++i) {
    //     const node = new Node();
    //     node.set_name(`test_node_${i}`);
    //     console.log(node.get_name());
    // }
    --times;
    if (times == 0) {
        clearInterval(timer_id);
        console.log("stop tick");
    }
}, 1000);
setImmediate(function () {
    console.log("immediate");
});
let infinite_loops = 0;
setInterval(function () {
    ++infinite_loops;
    console.log("a test interval timer never stop 1", infinite_loops);
}, 5000);
setTimeout(function (...args) {
    console.log("timeout", ...args);
    if (jsb.DEV_ENABLED) {
        console.log("it's a debug build");
    }
    else {
        console.log("it's a release build");
    }
}, 1500, 123, 456.789, "abc");
function delay(seconds) {
    return __awaiter(this, void 0, void 0, function* () {
        return new Promise(function (resolve) {
            setTimeout(function () {
                resolve();
            }, seconds * 1000);
        });
    });
}
function test_async() {
    return __awaiter(this, void 0, void 0, function* () {
        console.log(godot_1.Engine.get_time_scale());
        console.log("async test begin");
        yield delay(3);
        console.log("async test end");
    });
}
test_async();
if (jsb.TOOLS_ENABLED) {
    let tsd = new tsd_codegen_1.TSDCodeGen("./hello.txt");
    tsd.emit();
}
console.log("c++ binding test");
console.log(typeof Vector3);
const v1 = new Vector3(1, 1, 1);
const v2 = new Vector3(2, 3, 4);
console.log("dot", v2.dot(v1));
const v3 = v1.move_toward(v2, 0.5);
console.log(v3.x, v3.y, v3.z);
//# sourceMappingURL=main_entry.js.map