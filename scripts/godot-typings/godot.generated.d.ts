// AUTO-GENERATED

declare module "godot" {
    class Node {

    }

    // singleton
    namespace Engine {
        function get_time_scale(): number;
    }

    class FileAccess {
		static readonly READ = 1
		static readonly WRITE = 2
		static readonly READ_WRITE = 3
		static readonly WRITE_READ = 7

        static open(path: string, flags: number)
        store_line(str: string);
        flush();
    }
}
