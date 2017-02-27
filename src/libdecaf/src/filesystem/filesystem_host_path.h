#pragma once
#include "filesystem_path.h"
#include <common/platform.h>

namespace fs
{

#ifdef PLATFORM_WINDOWS
using HostPath = GenericPath<'\\'>;
#else
using HostPath = GenericPath<'/'>;
#endif

} // namespace fs
