"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ConcreteClass = exports.AbstractClass = exports.deleteMe = exports.DeleteMe = void 0;
const godot_1 = require("godot");
class DeleteMe {
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
    foo() {
        this.bark();
    }
}
exports.ConcreteClass = ConcreteClass;
class LocalNode extends godot_1.Node {
}
class SubLocalNode extends LocalNode {
}
class TestClass extends SubLocalNode {
}
exports.default = TestClass;
//# sourceMappingURL=scratchpad.js.map