
// godot types will be exposed to JS runtime until they are actually used
import { Node, Engine } from "godot";
import { TSDCodeGen } from "./tsd_codegen";

// entry point (editor only)

// log, warn, error, assert are essentially supported
console.log("main entry in editor");
console.assert(typeof Node === "function");

let timer_id : any;
let times = 5;

// setInterval, setTimeout, setImmediate, clearInterval, clearTimeout are essentially supported
// NOTE: they return a timer_id which is an integer instead of Timeout object like in NodeJS
timer_id = setInterval(function () {
    console.log("tick");
    --times;
    if (times == 0) {
        clearInterval(timer_id);
        console.log("stop tick");
    }
}, 1000);

setImmediate(function(){
    console.log("immediate");
});

let infinite_loops = 0;
setInterval(function () {
    ++infinite_loops;
    console.log("a test interval timer never stop 1", infinite_loops);
}, 5000);

setTimeout(function(...args){
    console.log("timeout", ...args);

    if (jsb.DEV_ENABLED) {
        console.log("it's a debug build");
    } else {
        console.log("it's a release build");
    }

    const node = new Node();
    node.set_name("test_node");
    console.log(node.get_name());

}, 1500, 123, 456.789, "abc");

async function delay(seconds: number) {
    return new Promise(function (resolve: (value?: any) => void) {
        setTimeout(function () {
            resolve();
        }, seconds * 1000);
    });
}

async function test_async() {
    console.log(Engine.get_time_scale());
    console.log("async test begin");
    await delay(3);
    console.log("async test end");
}

test_async();

if (jsb.TOOLS_ENABLED) {
    let tsd = new TSDCodeGen("./hello.txt");
    tsd.emit();
}
