#pragma once
#include "libcpu/cpu.h"
#include "libcpu/espresso/espresso_instructionid.h"

namespace cpu
{

namespace jit
{

/**
 * Initialize the JIT compiler with the given cache size (in bytes) for
 * translated code.
 */
void
initialise(size_t cacheSize);

/**
 * Initialize JIT-related fields in a Core instance.
 */
void
initCore(Core *core);

/**
 * Clear the JIT cache.  This function must not be called while any JIT code
 * is being executed.
 */
void
clearCache();

/**
 * Set optimization flags for generating native code.  See sOptFlags in
 * jit.cpp for a full list.
 */
void
setOptFlags(const std::vector<std::string> &optList);

/**
 * Mark the given range of addresses as read-only for JIT optimization.
 */
void
addReadOnlyRange(ppcaddr_t start, uint32_t size);

/**
 * Begin executing guest code on the current core.
 */
void
resume();

} // namespace jit

} // namespace cpu
