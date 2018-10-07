#pragma once
#include "latte/latte_enum_cp.h"
#include "latte/latte_registers_cp.h"

#include <cstdint>
#include <gsl.h>

namespace gpu::ih
{

struct Entry
{
   uint32_t word0;
   uint32_t word1;
   uint32_t word2;
   uint32_t word3;
};

using Entries = gsl::span<const Entry>;
using InterruptCallbackFn = void(*) ();

void
write(const Entries &entries);

inline void
write(const Entry &entry)
{
   write({ &entry, 1 });
}

Entries
read();

void
setInterruptCallback(InterruptCallbackFn callback);

void
enable(latte::CP_INT_CNTL cntl);

void
disable(latte::CP_INT_CNTL cntl);

} // namespace gpu::ih
