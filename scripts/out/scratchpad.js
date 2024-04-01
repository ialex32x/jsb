"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ConcreteClass = exports.AbstractClass = exports.deleteMe = exports.DeleteMe = void 0;
const godot_1 = require("godot");
class DeleteMe {
    constructor() {
        console.trace("test trace");
    }
}
exports.DeleteMe = DeleteMe;
;
exports.deleteMe = 1;
class AbstractClass {
    bark() {
        console.log("bark at the moon");
    }
}
exports.AbstractClass = AbstractClass;
class ConcreteClass extends AbstractClass {
    foo() { this.bark(); }
}
exports.ConcreteClass = ConcreteClass;
(function () {
    const temp = new godot_1.Node();
})();
class LocalNode extends godot_1.Node {
}
class SubLocalNode extends LocalNode {
}
class TestClass extends SubLocalNode {
    constructor() {
        console.log("test class constructor calling");
        super();
        console.log("test class constructor called");
    }
}
exports.default = TestClass;
console.log("c++ binding test");
console.log(typeof godot_1.Vector3);
const v1 = new godot_1.Vector3(1, 1, 1);
const v2 = new godot_1.Vector3(2, 3, 4);
console.log("dot", v2.dot(v1));
const v3 = v1.move_toward(v2, 0.5);
console.log(v3.x, v3.y, v3.z);
//# sourceMappingURL=scratchpad.js.map