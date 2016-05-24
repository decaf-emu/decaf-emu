#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "espresso_instruction.h"

namespace espresso
{

struct InstructionInfo;

struct Disassembly
{
   struct Argument
   {
      enum Type
      {
         Invalid,
         Address,
         Register,
         ValueUnsigned,
         ValueSigned,
         ConstantUnsigned,
         ConstantSigned
      };

      Type type;
      std::string text;

      union
      {
         uint32_t address;
         uint32_t constantUnsigned;
         int32_t constantSigned;
         uint32_t valueUnsigned;
         int32_t valueSigned;
      };
   };

   uint32_t address;
   InstructionInfo *instruction;
   std::string name;
   std::vector<Argument> args;
   std::string text;
};

bool disassemble(Instruction bin, Disassembly &out, uint32_t address);

} // namespace espresso
