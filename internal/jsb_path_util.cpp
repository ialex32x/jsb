#include "jsb_path_util.h"

// std::size
#include <iterator>

#include "core/variant/variant.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"


namespace jsb::internal
{
    String PathUtil::dirname(const String& p_name)
    {
        int index = p_name.rfind("/");
        if (index < 0)
        {
            index = p_name.rfind("\\");
            if (index < 0)
            {
                return p_name;
            }
        }
        return p_name.substr(0, index);
    }

    Error PathUtil::extract(const String& p_path, String& o_path)
    {
        static String same_level = ".";
        static String upper_level = "..";
        static String sp = "/";

        Vector<String> components = p_path.split(sp);
        int index = components.size() - 1;

        while (index >= 0)
        {
            const String& component = components[index];
            if (component == same_level)
            {
                components.remove_at(index--);
                ERR_FAIL_COND_V(index < 0 || index >= components.size(), ERR_FILE_BAD_PATH);
                continue;
            }
            if (component == upper_level)
            {
                components.remove_at(index--);
                ERR_FAIL_COND_V(index < 0 || index >= components.size(), ERR_FILE_BAD_PATH);
                components.remove_at(index--);
                ERR_FAIL_COND_V(index < 0 || index >= components.size(), ERR_FILE_BAD_PATH);
                continue;
            }
            --index;
        }

        o_path.clear();
        for (int i = 0, n = components.size(); i < n; ++i)
        {
            const String& component = components[i];
            if (i != n - 1)
            {
                o_path += component + '/';
            }
            else
            {
                o_path += component;
            }
        }
        return OK;
    }


    bool PathUtil::find(const String& p_path)
    {
        const Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
        return da->file_exists(p_path);
    }

    String PathUtil::get_full_path(const String& p_path)
    {
        //TODO
        return p_path;
    }

    Vector<uint8_t> PathUtil::read_all_bytes(const String& p_path, EFileTransform::Type p_transform)
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

        if (p_transform == EFileTransform::CommonJS)
        {
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

        Vector<uint8_t> data;
        data.resize((int) f->get_length());
        f->get_buffer(data.ptrw(), data.size());
        return data;
    }
}
