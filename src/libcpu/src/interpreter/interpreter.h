#pragma once
#include "cpu.h"

namespace cpu
{

namespace interpreter
{

void
initialise();

Core *
step_one(Core *core);

void
resume();

} // namespace interpreter

} // namespace cpu
