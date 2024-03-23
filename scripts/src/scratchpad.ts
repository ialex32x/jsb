import { Node } from "godot";

export class DeleteMe { };
export const deleteMe = 1;

export abstract class AbstractClass {
    abstract foo(): void;

    bark(): void { 
        console.log("bark at the moon")
    }
}

export class ConcreteClass extends AbstractClass {
    foo(): void {
        this.bark();
    }
}

class LocalNode extends Node { }
class SubLocalNode extends LocalNode { }

export default class TestClass extends SubLocalNode {

}
