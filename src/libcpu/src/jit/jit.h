#pragma once
#include "jit_backend.h"

namespace cpu
{

namespace jit
{

JitBackend *
getBackend();

void
setBackend(JitBackend *backend);

Core *
initialiseCore(uint32_t id);

void
clearCache(uint32_t address, uint32_t size);

void
addReadOnlyRange(uint32_t address, uint32_t size);

void
resume();

} // namespace jit

} // namespace cpu
