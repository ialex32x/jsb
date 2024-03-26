#ifndef JAVASCRIPT_PCH_H
#define JAVASCRIPT_PCH_H

#include <v8.h>
#include <v8-persistent-handle.h>
#include <libplatform/libplatform.h>
#include <v8-inspector.h>
#include <v8-version-string.h>

#include <cstdint>

#include "core/core_constants.h"
#include "core/string/string_builder.h"

#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/math/vector4.h"

#include "../internal/jsb_macros.h"
#include "../internal/jsb_sarray.h"
#include "../internal/jsb_function_pointer.h"

#endif
