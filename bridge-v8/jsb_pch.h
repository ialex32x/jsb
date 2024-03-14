#ifndef JAVASCRIPT_PCH_H
#define JAVASCRIPT_PCH_H

#include <v8.h>
#include <v8-persistent-handle.h>
#include <libplatform/libplatform.h>
#include <cstdint>

#include "core/object/object.h"
#include "core/string/print_string.h"
#include "core/templates/hash_map.h"
#include "core/io/file_access.h"
#include "core/io/dir_access.h"
#include "core/os/thread.h"
#include "core/os/os.h"

#include "../internal/jsb_macros.h"

#endif
