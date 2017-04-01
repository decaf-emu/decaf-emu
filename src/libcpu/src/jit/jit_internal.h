#pragma once
#include <cstddef>

#define offsetof2(s, m) ((size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))

namespace cpu
{

namespace jit
{

extern unsigned int
gCommonOpt;

extern unsigned int
gGuestOpt;

extern unsigned int
gHostOpt;

} // namespace jit

} // namespace cpu
