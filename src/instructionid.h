#pragma once

#define INS(x, ...) x,
#define INSA(...)

enum class InstructionID
{
   #include "instructions.inl"
   Invalid,
   InstructionCount
};

#undef INS
#undef INSA
