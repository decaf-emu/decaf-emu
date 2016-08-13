#pragma once
#include "common/types.h"

namespace coreinit
{

BOOL
OSIsDebuggerPresent();

BOOL
OSIsDebuggerInitialized();

int
ENVGetEnvironmentVariable(const char *key,
                          char *buffer,
                          uint32_t length);

} // namespace coreinit
