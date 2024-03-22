
declare module globalThis {
    class Vector3 {
        x: number;
        y: number;
        z: number;

        constructor(x: number, y: number, z: number);
        dot(a: Vector3): number;
        move_toward(a: Vector3, l: number): Vector3;
    }
}
