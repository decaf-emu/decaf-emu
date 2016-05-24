#pragma once

namespace espresso
{

#define INS(x, ...) x,
enum class InstructionID
{
#  include "espresso_instruction_definitions.inl"
   Invalid,
   InstructionCount
};
#undef INS

} // namespace espresso
