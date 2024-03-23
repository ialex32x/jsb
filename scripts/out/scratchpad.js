"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ConcreteClass = exports.AbstractClass = exports.deleteMe = exports.DeleteMe = void 0;
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
//# sourceMappingURL=scratchpad.js.map