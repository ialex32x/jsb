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

#if DEV_ENABLED
#   define JSB_LOG(Severity, Format, ...) print_line(vformat("[%s] " Format, ((void) sizeof(jsb::internal::ELogSeverity::Severity), #Severity), ##__VA_ARGS__))
#   define jsb_check(Condition) CRASH_COND(!(Condition))
#   define jsb_checkf(Condition, Message) CRASH_COND_MSG(!(Condition), (Message))
#   define jsb_ensure(Condition) CRASH_COND(!(Condition))
#else
#   define JSB_LOG(Severity, Format, ...) (void) 0
#   define jsb_check(Condition) (void) 0
#   define jsb_checkf(Condition, Message) (void) 0
#   define jsb_ensure(Condition) CRASH_COND(!(Condition))
#endif

#define jsb_typename(TypeName) ((void) sizeof(TypeName), #TypeName)
#define jsb_nameof(TypeName, MemberName) ((void) sizeof(TypeName::MemberName), #MemberName)
#define jsb_methodbind(TypeName, MemberName) &TypeName::MemberName, #MemberName

#define jsb_downscale(v64) ::jsb::internal::downscale(v64)
#define jsb_downscalef(v64, v_msg) ::jsb::internal::downscale(v64, v_msg)

#define JSB_CONSOLE(Severity, Message) print_line(Severity, Message)

#if defined(__GNUC__) || defined(__clang__)
#   define jsb_force_inline  __attribute__((always_inline))
#elif defined(_MSC_VER)
#   define jsb_force_inline  __forceinline
#else
#   define jsb_force_inline
#endif

#define jsb_no_discard [[nodiscard]]

#if !defined(JSB_DISABLE_STACKALLOC)
#   define jsb_stackalloc(size) alloca(size)
#   define jsb_stackfree(ptr)
#else
#   define jsb_stackalloc(size) memalloc(size)
#   define jsb_stackfree(ptr) memfree(ptr)
#endif

namespace jsb::internal
{
    namespace ELogSeverity
    {
        enum Type
        {
#pragma push_macro("DEF")
#   undef   DEF
#   define  DEF(FieldName) FieldName,
#   include "jsb_log_severity.h"
#pragma pop_macro("DEF")
        };
    }

    template<typename T>
    jsb_force_inline int32_t downscale(int64_t p_val, const T& p_msg)
    {
#if DEV_ENABLED
        if (p_val != (int64_t) (int32_t) p_val) JSB_LOG(Warning, "inconsistent int64_t conversion: %s", p_msg);
#endif
        return (int32_t) p_val;
    }

    jsb_force_inline int32_t downscale(int64_t p_val)
    {
#if DEV_ENABLED
        if (p_val != (int64_t) (int32_t) p_val) JSB_LOG(Warning, "inconsistent int64_t conversion");
#endif
        return (int32_t) p_val;
    }
}

#endif
