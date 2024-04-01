#include "jsb_module_loader.h"
#include "jsb_context.h"

namespace jsb
{
    bool GodotModuleLoader::load(class JavaScriptContext* p_ccontext, JavaScriptModule& p_module)
    {
        static const CharString on_demand_loader_source = ""
            "(function (type_loader_func) {"
            "    return (function (type_obj, type_name) {"
            "        if (typeof type_obj === 'undefined') {"
            "            throw new Error(`type '${type_name}' does not exist`);"
            "        }"
            "        if (typeof type_obj !== 'object') {"
            "            return type_obj;"
            "        }"
            "        return new Proxy(type_obj, {"
            "            set: function (target, prop_name, value) {"
            "                if (typeof prop_name !== 'string') {"
            "                    throw new Error(`only string key is allowed`);"
            "                }"
            "                if (typeof target[prop_name] !== 'undefined') {"
            "                    target[prop_name] = value;"
            "                    return true;"
            "                }"
            "                let type_path = typeof type_name === 'string' ? type_name + '.' + prop_name : prop_name;"
            "                throw new Error(type_path + ' is inaccessible');"
            "            },"
            "            get: function (target, prop_name) {"
            "                let o = target[prop_name];"
            "                if (typeof o === 'undefined' && typeof prop_name === 'string') {"
            "                    let type_path = typeof type_name === 'string' ? type_name + '.' + prop_name : prop_name;"
            "                    o = target[prop_name] = type_loader_func(type_path);"
            "                }"
            "                return o;"
            "            }"
            "        });"
            "    })({});"
            "})"
            "";
        v8::Isolate* isolate = p_ccontext->get_isolate();
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Context::Scope context_scope(context);

        jsb_check(p_ccontext->check(context));
        v8::MaybeLocal<v8::Value> func_maybe_local = p_ccontext->_compile_run(on_demand_loader_source, "on_demand_loader_source");
        if (func_maybe_local.IsEmpty())
        {
            return false;
        }

        v8::Local<v8::Value> func_local;
        if (!func_maybe_local.ToLocal(&func_local) || !func_local->IsFunction())
        {
            isolate->ThrowError("not a function");
            return false;
        }

        // load_type_impl: function(name)
        v8::Local<v8::Value> argv[] = { v8::Function::New(context, JavaScriptContext::_load_godot_mod).ToLocalChecked() };
        v8::Local<v8::Function> loader = func_local.As<v8::Function>();
        v8::MaybeLocal<v8::Value> type_maybe_local = loader->Call(context, v8::Undefined(isolate), std::size(argv), argv);
        if (type_maybe_local.IsEmpty())
        {
            return false;
        }

        // it's a proxy object which will load godot type on-demand until it's actually accessed in a script
        v8::Local<v8::Value> type_local;
        if (!type_maybe_local.ToLocal(&type_local) || !type_local->IsObject())
        {
            isolate->ThrowError("bad call");
            return false;
        }

        p_module.exports.Reset(isolate, type_local.As<v8::Object>());
        return true;
    }

}
