import { Node, Vector3 } from "godot";

export class DeleteMe {
    constructor() {
        console.trace("test trace");
    }
};
export const deleteMe = 1;
export abstract class AbstractClass {
    abstract foo(): void;
    bark(): void {
        console.log("bark at the moon")
    }
}
export class ConcreteClass extends AbstractClass {
    foo(): void { this.bark(); }
}
(function () {
    const temp = new Node();
})();
class LocalNode extends Node { }
class SubLocalNode extends LocalNode { }
export default class TestClass extends SubLocalNode {
    constructor() {
        console.log("test class constructor calling");
        super();
        console.log("test class constructor called");
    }
}


console.log("c++ binding test");
console.log(typeof Vector3);
console.assert(Vector3.Axis.AXIS_X == 0);
const v1 = new Vector3(1, 1, 1);
const v2 = new Vector3(2, 3, 4);
console.log("dot", v2.dot(v1));

const v3 = v1.move_toward(v2, 0.5)
console.log(v3.x, v3.y, v3.z)
