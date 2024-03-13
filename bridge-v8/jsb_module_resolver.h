#ifndef JAVASCRIPT_MODULE_RESOLVER_H
#define JAVASCRIPT_MODULE_RESOLVER_H

#include "jsb_pch.h"
#include "jsb_module.h"

namespace jsb
{
    class IModuleResolver
    {
    public:
        virtual ~IModuleResolver() = default;

        // check if the given module_id is valid to resolve
        virtual bool is_valid(const String& p_module_id) = 0;

        // load source into the module
        virtual bool load(class JavaScriptContext* p_ccontext, JavaScriptModule& p_module) = 0;
    };

    class SourceModuleResolver : public IModuleResolver
    {
    public:
        virtual ~SourceModuleResolver() override = default;

        virtual bool is_valid(const String& p_module_id) override;
        virtual bool load(class JavaScriptContext* p_ccontext, JavaScriptModule& p_module) override;

    private:
        // read the source buffer (transformed into commonjs)
        Vector<uint8_t> read_all_bytes(const String& p_path);
    };
}

#endif
