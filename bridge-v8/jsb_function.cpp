#include "jsb_function.h"

#include "jsb_context.h"
#include "jsb_exception_info.h"

namespace jsb
{
    Variant JavaScriptFunction::call(JavaScriptContext* p_ccontext, NativeObjectID p_object_id, const Variant** p_args, int p_argcount, Callable::CallError& r_error)
    {
        v8::Isolate* isolate = p_ccontext->get_isolate();
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = p_ccontext->unwrap();
        v8::Context::Scope context_scope(context);
        v8::Local<v8::Function> func = function_.Get(isolate);
        v8::Local<v8::Value> self = p_object_id.is_valid() ? p_ccontext->runtime_->objects_.get_value(p_object_id).ref_.Get(isolate) : v8::Undefined(isolate);

        v8::TryCatch try_catch_run(isolate);
        using LocalValue = v8::Local<v8::Value>;
        LocalValue* argv = jsb_stackalloc(LocalValue, p_argcount);
        for (int index = 0; index < p_argcount; ++index)
        {
            memnew_placement(&argv[index], LocalValue);
            if (!JavaScriptContext::gd_var_to_js(isolate, context, *p_args[index], argv[index]))
            {
                // revert constructed values if error occured
                while (index >= 0) argv[index--].~LocalValue();
                r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
                return {};
            }
        }
        v8::MaybeLocal<v8::Value> rval = func->Call(context, self, p_argcount, argv);
        for (int index = 0; index < p_argcount; ++index)
        {
            argv[index].~LocalValue();
        }
        if (JavaScriptExceptionInfo exception_info = JavaScriptExceptionInfo(isolate, try_catch_run))
        {
            r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
            return {};
        }

        if (rval.IsEmpty())
        {
            return {};
        }

        Variant rvar;
        if (!JavaScriptContext::js_to_gd_var(isolate, context, rval.ToLocalChecked(), rvar))
        {
            r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
            return {};
        }
        return rvar;
    }

}
