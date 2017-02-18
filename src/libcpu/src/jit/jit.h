#pragma once
#include "libcpu/cpu.h"
#include "libcpu/espresso/espresso_instructionid.h"

namespace cpu
{

namespace jit
{

void
initialise();

void
clearCache();

void
resume();

bool
hasInstruction(espresso::InstructionID instrId);

} // namespace jit

} // namespace cpu
