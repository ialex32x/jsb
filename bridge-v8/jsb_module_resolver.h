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
        // `r_asset_path` will be set if there is an available source file in `FileAccess` which matches the given `module_id`
        virtual bool get_source_info(const String& p_module_id, String& r_asset_path) = 0;

        // load source into the module
        // `exports' will be set into `p_module.exports` if loaded successfully
        virtual bool load(class JavaScriptContext* p_ccontext, const String& r_asset_path, JavaScriptModule& p_module) = 0;
    };

    // the default module resolver finds source files directly with `FileAccess` with `search_paths`
    class DefaultModuleResolver : public IModuleResolver
    {
    public:
        virtual ~DefaultModuleResolver() override = default;

        virtual bool get_source_info(const String& p_module_id, String& r_asset_path) override;
        virtual bool load(class JavaScriptContext* p_ccontext, const String& r_asset_path, JavaScriptModule& p_module) override;

        DefaultModuleResolver& add_search_path(const String& p_path);

    protected:
        virtual Ref<FileAccess> get_file_access()
        {
            if (file_access_.is_null())
            {
                file_access_ = FileAccess::create(FileAccess::ACCESS_RESOURCES);
            }
            return file_access_;
        }

        virtual Ref<FileAccess> open_file(const String &p_path, int p_mode_flags, Error *r_error)
        {
            return FileAccess::open(p_path, p_mode_flags, r_error);
        }

        // read the source buffer (transformed into commonjs)
        Vector<uint8_t> read_all_bytes(const String& r_asset_path, String* r_filename);

        Ref<FileAccess> file_access_;
        Vector<String> search_paths_;
    };
}

#endif
