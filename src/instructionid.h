#pragma once

#define INS(x, ...) x,

enum class InstructionID
{
   #include "instructions.inl"
   Invalid,
   InstructionCount
};

#undef INS
