#ifndef JAVASCRIPT_PATH_UTIL_H
#define JAVASCRIPT_PATH_UTIL_H

#include "core/string/ustring.h"

namespace jsb::internal
{
    namespace EFileTransform
    {
        enum Type
        {
            None,
            CommonJS,
        };
    };

    class PathUtil
    {
    public:
        static String combine(const String& p_base, const String& p_add)
        {
            if (p_base.ends_with("/"))
            {
                return p_base + p_add;
            }
            return p_base + '/' + p_add;
        }

        static String dirname(const String& p_name);

        // extract the relative path (eliminate all '.' and '..')
        static Error extract(const String& p_path, String& o_path);

        static bool find(const String& p_path);
        static String get_full_path(const String& p_path);
        static Vector<uint8_t> read_all_bytes(const String& p_path, EFileTransform::Type p_transform);
    };
}
#endif
