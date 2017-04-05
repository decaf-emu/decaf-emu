#pragma once
#include <common/cbool.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_debug Debug
 * \ingroup coreinit
 * @{
 */

BOOL
OSIsDebuggerPresent();

BOOL
OSIsDebuggerInitialized();

int
ENVGetEnvironmentVariable(const char *key,
                          char *buffer,
                          uint32_t length);

/** @} */

} // namespace coreinit
