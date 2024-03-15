#include "jsb_timer_action.h"
#include "jsb_exception_info.h"
#include "jsb_context.h"

namespace jsb
{
    void JavaScriptTimerAction::operator()(v8::Isolate* isolate)
    {
        v8::Local<v8::Function> func = function_.Get(isolate);
        v8::Local<v8::Context> context = func->GetCreationContextChecked();
        JavaScriptContext* ccontext = JavaScriptContext::wrap(context);

        if (!ccontext)
        {
            JSB_LOG(Warning, "timer triggered after JavaScriptContext diposed");
            return;
        }
        v8::Context::Scope context_scope(context);

        v8::TryCatch try_catch(isolate);
        if (argc_ > 0)
        {
            using LocalValue = v8::Local<v8::Value>;
            LocalValue* argv = (LocalValue*)jsb_stackalloc(sizeof(LocalValue) * argc_);
            memset((void*) argv, 0, sizeof(LocalValue) * argc_);

            for (int index = 0; index < argc_; ++index)
            {
                argv[index] = argv_[index].Get(isolate);
            }
            func->Call(context, v8::Undefined(isolate), argc_, argv);
            for (int index = 0; index < argc_; ++index)
            {
                argv[index].~LocalValue();
            }
            jsb_stackfree(argv);
        }
        else
        {
            func->Call(context, v8::Undefined(isolate), 0, nullptr);
        }

        if (JavaScriptExceptionInfo exception_info = JavaScriptExceptionInfo(isolate, try_catch))
        {
            JSB_LOG(Error, "timer error %s", (String) exception_info);
        }
    }
}
