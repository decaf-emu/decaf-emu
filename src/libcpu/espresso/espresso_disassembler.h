#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "espresso_instruction.h"
#include "espresso_instructionid.h"

namespace espresso
{

struct InstructionInfo;

struct BranchInfo
{
   bool isVariable;
   uint32_t target;
   bool isConditional;
   bool conditionSatisfied;
   bool isCall;
};

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
BranchInfo disassembleBranchInfo(InstructionID id, Instruction ins,
                                 uint32_t address, uint32_t ctr,
                                 uint32_t cr, uint32_t lr);

} // namespace espresso
