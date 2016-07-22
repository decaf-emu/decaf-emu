#pragma once
#include "jit.h"

namespace cpu
{

namespace jit
{

void
roundToSingleSd(PPCEmuAssembler& a,
                const PPCEmuAssembler::XmmRegister& dst,
                const PPCEmuAssembler::XmmRegister& src);

void
truncateToSingleSd(PPCEmuAssembler& a,
                   const PPCEmuAssembler::XmmRegister& dst,
                   const PPCEmuAssembler::XmmRegister& src);

} // namespace jit

} // namespace cpu
