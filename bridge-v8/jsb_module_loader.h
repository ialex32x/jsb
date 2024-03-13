#ifndef JAVASCRIPT_MODULE_LOADER_H
#define JAVASCRIPT_MODULE_LOADER_H

#include "jsb_pch.h"
#include "jsb_module.h"

namespace jsb
{
    // resolve module with a name already known when registering to runtime
    class IModuleLoader
    {
    public:
        virtual ~IModuleLoader() = default;

        virtual bool load(class JavaScriptContext* p_ccontext, JavaScriptModule& p_module) = 0;
    };

    class GodotModuleLoader : public IModuleLoader
    {
    public:
        virtual ~GodotModuleLoader() override = default;

        virtual bool load(class JavaScriptContext* p_ccontext, JavaScriptModule& p_module) override;
    };
}
#endif
