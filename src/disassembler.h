#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "instruction.h"

struct InstructionData;

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
   InstructionData *instruction;
   std::string name;
   std::vector<Argument> args;
   std::string text;
};

struct Disassembler
{
   bool disassemble(Instruction bin, Disassembly &out, uint32_t address);
};

extern Disassembler gDisassembler;
