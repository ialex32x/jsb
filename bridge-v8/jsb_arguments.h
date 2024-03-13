#ifndef JAVASCRIPT_ARGUMENTS_H
#define JAVASCRIPT_ARGUMENTS_H

#include "jsb_pch.h"

namespace jsb
{
    struct JavaScriptArguments
    {
    private:
        Variant *args;

    public:
        jsb_force_inline Variant& operator[](int p_index)
        {
            return args[p_index];
        }

        jsb_force_inline JavaScriptArguments(int p_argc)
        {
            if (p_argc == 0)
            {
                args = nullptr;
            }
            else
            {
                args = memnew_arr(Variant, p_argc);
            }
        }

        jsb_force_inline ~JavaScriptArguments()
        {
            if (args)
            {
                memdelete_arr(args);
            }
        }
    };
}

#endif
