#ifndef JAVASCRIPT_MODULE_H
#define JAVASCRIPT_MODULE_H

#include "jsb_pch.h"

namespace jsb
{
    struct JavaScriptModule
    {
        String id;

        v8::Global<v8::Object> module;
        v8::Global<v8::Object> exports;

    };

    struct JavaScriptModuleCache
    {
        jsb_force_inline ~JavaScriptModuleCache()
        {
            for (KeyValue<String, JavaScriptModule*>& it : module_cache_)
            {
                memdelete(it.value);
            }
        }

        jsb_force_inline JavaScriptModule* find(const String& p_name) const
        {
            const HashMap<String, JavaScriptModule*>::ConstIterator it = module_cache_.find(p_name);
            return it != module_cache_.end() ? it->value : nullptr;
        }

        jsb_force_inline JavaScriptModule& insert(const String& p_name, bool p_main_candidate)
        {
            jsb_checkf(!p_name.is_empty(), "empty module name is not allowed");
            jsb_checkf(!find(p_name), "duplicated module name %s", p_name);
            if (p_main_candidate && main_.is_empty())
            {
                main_ = p_name;
                JSB_LOG(Verbose, "load main module %s", p_name);
            }
            else
            {
                JSB_LOG(Verbose, "loading module %s", p_name);
            }
            JavaScriptModule* module = memnew(JavaScriptModule);
            module_cache_.insert(p_name, module);
            return *module;
        }

        jsb_force_inline JavaScriptModule* get_main() const
        {
            return main_.is_empty() ? nullptr : find(main_);
        }

        jsb_force_inline bool is_main(const String& p_name) const
        {
            return p_name == main_;
        }

        static jsb_force_inline String get_name(v8::Isolate* isolate, const v8::Local<v8::Value>& p_value)
        {
            if (p_value->IsString())
            {
                v8::String::Utf8Value decode(isolate, p_value);
                return String(*decode, decode.length());
            }
            return String();
        }

    private:
        String main_;
        HashMap<String, JavaScriptModule*> module_cache_;
    };

}

#endif
