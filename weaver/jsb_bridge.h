#ifndef JSB_BRIDGE_H
#define JSB_BRIDGE_H

#if JSB_WITH_V8
#   include "../bridge-v8/jsb_runtime.h"
#   include "../bridge-v8/jsb_context.h"
#   include "../bridge-v8/jsb_exception_info.h"
#   include "../bridge-v8/jsb_class_info.h"
#   include "../bridge-v8/jsb_class_instance.h"
#elif JSB_WITH_QUICKJS
#   include "../bridge-quickjs/jsb_runtime.h"
#   error "not implemented"
#else
#   error "unknown javascript runtime"
#endif

#endif
