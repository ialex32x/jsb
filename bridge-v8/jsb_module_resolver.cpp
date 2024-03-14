#include "jsb_module_resolver.h"

#include "jsb_context.h"
#include "../internal/jsb_path_util.h"

namespace jsb
{
    Vector<uint8_t> DefaultModuleResolver::read_all_bytes(const String& r_asset_path, String* r_filename)
    {
        Error r_error;
        const Ref<FileAccess> f = open_file(r_asset_path, FileAccess::READ, &r_error);
        if (r_filename)
        {
            *r_filename = f->get_path_absolute();
        }
        if (f.is_null())
        {
            if (!r_error)
            {
                return Vector<uint8_t>();
            }
            ERR_FAIL_V_MSG(Vector<uint8_t>(), "Can't open file from path '" + String(r_asset_path) + "'.");
        }

        constexpr static char header[] = "(function(exports,require,module,__filename,__dirname){";
        constexpr static char footer[] = "\n})";

        Vector<uint8_t> data;
        const size_t file_len = f->get_length();
        data.resize((int) (file_len + ::std::size(header) + ::std::size(footer) - 2));

        memcpy(data.ptrw(), header, ::std::size(header) - 1);
        f->get_buffer(data.ptrw() + ::std::size(header) - 1, file_len);
        memcpy(data.ptrw() + file_len + ::std::size(header) - 1, footer, ::std::size(footer) - 1);
        return data;
    }

    // early and simple validation: check source file existence
    bool DefaultModuleResolver::get_source_info(const String &p_module_id, String& r_asset_path)
    {
        String extended = p_module_id + ".js";
        for (const String& search_path : search_paths_)
        {
            String filename = internal::PathUtil::combine(search_path, extended);
            if (get_file_access()->file_exists(filename))
            {
                r_asset_path = filename;
                return true;
            }
        }
        r_asset_path.clear();
        return false;
    }

    DefaultModuleResolver& DefaultModuleResolver::add_search_path(const String& p_path)
    {
        search_paths_.append(p_path);
        return *this;
    }

    bool DefaultModuleResolver::load(JavaScriptContext *p_ccontext, const String& r_asset_path, JavaScriptModule &p_module)
    {
        v8::Isolate* isolate = p_ccontext->get_isolate();

        // load source buffer
        String filename_abs;
        const Vector<uint8_t> source = read_all_bytes(r_asset_path, &filename_abs);
        if (source.size() == 0)
        {
            isolate->ThrowError("failed to read module source");
            return false;
        }

        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = isolate->GetCurrentContext();
        v8::Context::Scope context_scope(context);

        jsb_check(p_ccontext->check(context));

        // failed to compile or run, immediately return since an exception should already be thrown
        v8::MaybeLocal<v8::Value> func_maybe_local = p_ccontext->_compile_run((const char*) source.ptr(), source.size(), "module_elevator");
        if (func_maybe_local.IsEmpty())
        {
            return false;
        }

        v8::Local<v8::Value> func_local;
        if (!func_maybe_local.ToLocal(&func_local) || !func_local->IsFunction())
        {
            isolate->ThrowError("bad module elevator");
            return false;
        }

        const CharString cmodule_id = p_module.id.utf8();
        const CharString cfilename = filename_abs.utf8();
        const CharString cdirname = internal::PathUtil::dirname(filename_abs).utf8();

        v8::Local<v8::Object> jexports = v8::Object::New(isolate);
        v8::Local<v8::Object> jmodule = p_module.module.Get(isolate);
        v8::Local<v8::String> jmodule_id = v8::String::NewFromUtf8(isolate, cmodule_id.ptr(), v8::NewStringType::kNormal, cmodule_id.length()).ToLocalChecked();
        v8::Local<v8::String> jfilename = v8::String::NewFromUtf8(isolate, cfilename.ptr(), v8::NewStringType::kNormal, cfilename.length()).ToLocalChecked();
        v8::Local<v8::Function> jrequire = v8::Function::New(context, JavaScriptContext::_require, /* magic: module_id */ jmodule_id).ToLocalChecked();
        v8::Local<v8::Function> elevator = func_local.As<v8::Function>();

        v8::Local<v8::Value> argv[] = { // exports, require, module, __filename, __dirname
            jexports,
            jrequire,
            jmodule,
            jfilename,
            v8::String::NewFromUtf8(isolate, cdirname.ptr(), v8::NewStringType::kNormal, cdirname.length()).ToLocalChecked(),
        };

        v8::Local<v8::Object> jmain_module;
        if (p_ccontext->_get_main_module(&jmain_module))
        {
            jrequire->Set(context, v8::String::NewFromUtf8Literal(isolate, "main"), jmain_module);
        }
        else
        {
            JSB_LOG(Warning, "invalid main module");
            jrequire->Set(context, v8::String::NewFromUtf8Literal(isolate, "main"), v8::Null(isolate));
        }
        p_module.exports.Reset(isolate, jexports);
        v8::MaybeLocal<v8::Value> type_maybe_local = elevator->Call(context, v8::Undefined(isolate), ::std::size(argv), argv);
        if (type_maybe_local.IsEmpty())
        {
            return false;
        }

        jmodule->Set(context, v8::String::NewFromUtf8Literal(isolate, "filename"), jfilename);
        return true;
    }

}
