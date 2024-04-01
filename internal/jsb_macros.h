#ifndef JSB_MACROS_H
#define JSB_MACROS_H

#include <cstdint>

#include "core/object/object.h"
#include "core/string/print_string.h"
#include "core/templates/hash_map.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "core/os/thread.h"
#include "core/os/os.h"

#ifndef JSB_MIN_LOG_LEVEL
#   define JSB_MIN_LOG_LEVEL Verbose
#endif

#if DEV_ENABLED
#   define JSB_LOG(Severity, Format, ...) if constexpr (jsb::internal::ELogSeverity::Severity >= jsb::internal::ELogSeverity::JSB_MIN_LOG_LEVEL) print_line(vformat("[%s] " Format, ((void) sizeof(jsb::internal::ELogSeverity::Severity), #Severity), ##__VA_ARGS__))
#   define jsb_check(Condition) CRASH_COND(!(Condition))
#   define jsb_checkf(Condition, Format, ...) CRASH_COND_MSG(!(Condition), vformat(Format, ##__VA_ARGS__))
#   define jsb_ensure(Condition) CRASH_COND(!(Condition))
#else
#   define JSB_LOG(Severity, Format, ...) (void) 0
#   define jsb_check(Condition) (void) 0
#   define jsb_checkf(Condition, Format, ...) (void) 0
#   define jsb_ensure(Condition) CRASH_COND(!(Condition))
#endif

#define jsb_typename(TypeName) ((void) sizeof(TypeName), #TypeName)
#define jsb_nameof(TypeName, MemberName) ((void) sizeof(TypeName::MemberName), #MemberName)
#define jsb_methodbind(TypeName, MemberName) &TypeName::MemberName, #MemberName
#define jsb_not_implemented(Condition, Format, ...) CRASH_COND_MSG((Condition), vformat(Format, ##__VA_ARGS__))

#define JSB_CONSOLE(Severity, Message) print_line(Severity, Message)

#if defined(__GNUC__) || defined(__clang__)
#   define jsb_force_inline  __attribute__((always_inline))
#elif defined(_MSC_VER)
#   define jsb_force_inline  __forceinline
#else
#   define jsb_force_inline
#endif

#define jsb_deprecated
#define jsb_deleteme
#define jsb_experimental
#define jsb_no_discard [[nodiscard]]

#define jsb_stackalloc(type, size) (type*) alloca(sizeof(type) * size)

namespace jsb::internal
{
    namespace ELogSeverity
    {
        enum Type : uint8_t
        {
#pragma push_macro("DEF")
#   undef   DEF
#   define  DEF(FieldName) FieldName,
#   include "jsb_log_severity.h"
#pragma pop_macro("DEF")
        };
    }
}

#endif
