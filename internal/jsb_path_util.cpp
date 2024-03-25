#include "jsb_path_util.h"

// std::size
#include <iterator>

#include "jsb_macros.h"
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
                // p_name is a pure filename without any dir hierarchy given
                return String();
            }
        }
        return p_name.substr(0, index);
    }

    Error PathUtil::extract(const String& p_path, String& o_path)
    {
#if 0
        // a tiny state machine to simplify the path
        enum State { Terminator, None, Component, Jumping, JumpSame, JumpUpper, Protocol, Failed,  };
        const int len = p_path.length();
        int read_cursor = len;
        char32_t* buf = (char32_t*) jsb_stackalloc((len + 1) * sizeof(char32_t));
        int write_cursor = len;
        int skip_componnets = 0;
        State state = Terminator;
        int slash = 0;
        while (state != Failed && read_cursor >= 0)
        {
            const char32_t ch = p_path.get(read_cursor--);
            switch (state)
            {
            case Component:
                if (ch == '/') { state = Jumping; ++slash; }
                if (!skip_componnets) { buf[write_cursor--] = ch; }
                break;
            case Jumping:
                if (ch == '.') { state = JumpSame; }
                else if (ch == '/')
                {
                    ++slash;
                    if (slash < 2)
                    {
                        buf[write_cursor--] = ch;
                    }
                    break;
                }
                else if (ch == ':')
                {
                    if (skip_componnets || slash != 2)
                    {
                        state = Failed; break;
                    }
                    buf[write_cursor--] = ch;
                    state = Protocol;
                }
                else
                {
                    state = Component;
                    if (skip_componnets == 0) { buf[write_cursor--] = ch; }
                    else skip_componnets = skip_componnets - 1;
                }
                slash = 0;
                break;
            case JumpSame:
                if (ch == '.') { state = JumpUpper; }
                else if (ch == '/') { state = Jumping; ++slash; }
                else
                {
                    // component ends with '.' is not allowed
                    state = Failed;
                }
                break;
            case JumpUpper:
                if (ch == '/') { state = Jumping; ++skip_componnets; ++slash; }
                else
                {
                    // only directory switch is allowed before '..'
                    state = Failed;
                }
                break;
            case Protocol:
                if (!is_ascii_alphanumeric_char(ch))
                {
                    state = Failed; break;
                }
                buf[write_cursor--] = ch;
                break;
            case Terminator:
                if (ch != '\0')
                {
                    state = Failed; break;
                }
                state = Component;
                buf[write_cursor--] = ch;
                break;
            default: jsb_check(false); break;
            }
        }

        if (state != Failed && skip_componnets == 0)
        {
            o_path = String(buf + write_cursor + 1, len - write_cursor);
            jsb_stackfree(buf);
            return OK;
        }
        jsb_stackfree(buf);
        return FAILED;
#else
        const static String same_level = ".";
        const static String upper_level = "..";
        const static String sp = "/";

        Vector<String> components = p_path.split(sp);
        int index = components.size() - 1;

        while (index >= 0)
        {
            const String& component = components[index];
            if (component == same_level)
            {
                components.remove_at(index--);
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
#endif
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
