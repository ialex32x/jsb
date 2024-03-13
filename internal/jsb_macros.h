#ifndef JSB_MACROS_H
#define JSB_MACROS_H

#include <cstdint>

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

#define JSB_CONSOLE(Severity, Message) print_line(Severity, Message)

#if defined(__GNUC__) || defined(__clang__)
#   define jsb_force_inline  __attribute__((always_inline))
#elif defined(_MSC_VER)
#   define jsb_force_inline  __forceinline
#else
#   define jsb_force_inline
#endif

namespace jsb::internal::ELogSeverity
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

#endif
