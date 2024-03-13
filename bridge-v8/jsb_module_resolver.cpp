#include "jsb_module_resolver.h"

#include "jsb_context.h"
#include "../internal/jsb_path_util.h"

namespace jsb
{
    Vector<uint8_t> SourceModuleResolver::read_all_bytes(const String& p_path)
    {
        Error r_error;
        const Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &r_error);
        if (f.is_null())
        {
            if (r_error)
            {
                return Vector<uint8_t>();
            }
            ERR_FAIL_V_MSG(Vector<uint8_t>(), "Can't open file from path '" + String(p_path) + "'.");
        }

        static constexpr char header[] = "(function(exports,require,module,__filename,__dirname){";
        static constexpr char footer[] = "\n})";

        Vector<uint8_t> data;
        const size_t file_len = f->get_length();
        data.resize((int) (file_len + std::size(header) + std::size(footer) - 2));

        memcpy(data.ptrw(), header, std::size(header) - 1);
        f->get_buffer(data.ptrw() + std::size(header) - 1, file_len);
        memcpy(data.ptrw() + file_len + std::size(header) - 1, footer, std::size(footer) - 1);
        return data;
    }

    bool SourceModuleResolver::is_valid(const String &p_module_id)
    {
        //TODO check source file existence
        //file_exists("res://" + p_module_id);
        return true;
    }

    bool SourceModuleResolver::load(JavaScriptContext *p_ccontext, JavaScriptModule &p_module)
    {
        v8::Isolate* isolate = p_ccontext->get_isolate();

        Vector<uint8_t> source = read_all_bytes("res://" + p_module.id);
        if (source.size() == 0)
        {
            isolate->ThrowError("failed to read module source");
            return false;
        }

        //TODO evaluate the module source
        // ...
        
        return false;
    }

}
