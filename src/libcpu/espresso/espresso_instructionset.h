#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "espresso_instruction.h"
#include "espresso_instructionid.h"

namespace espresso
{

#define FLD(x, ...) x,
#define MRKR(x, ...) x,
enum class InstructionField : uint32_t
{
   Invalid,
#  include "espresso_instruction_fields.inl"
   FieldCount,
};
#undef FLD
#undef MRKR

struct InstructionOpcode
{
   InstructionOpcode()
   {
   }

   InstructionOpcode(InstructionField field_, uint32_t value_) :
      field(field_),
      value(value_)
   {
   }

   InstructionOpcode(InstructionField field_, InstructionField field2_) :
      field(field_),
      field2(field2_)
   {
   }

   InstructionField field = InstructionField::Invalid;
   InstructionField field2 = InstructionField::Invalid;
   uint32_t value = 0;
};

struct InstructionInfo
{
   InstructionID id;
   std::string name;
   std::string fullname;
   std::vector<InstructionOpcode> opcode;
   std::vector<InstructionField> read;
   std::vector<InstructionField> write;
   std::vector<InstructionField> flags;
};

struct InstructionAlias
{
   std::string name;
   InstructionID id;
   std::vector<InstructionOpcode> opcode;
};

void
initialiseInstructionSet();

InstructionInfo *
decodeInstruction(Instruction instr);

Instruction
encodeInstruction(InstructionID id);

InstructionInfo *
findInstructionInfo(InstructionID instrId);

InstructionAlias *
findInstructionAlias(InstructionInfo *info, Instruction instr);

bool
isA(InstructionID id, Instruction instr);

template<InstructionID id>
bool
isA(Instruction instr)
{
   return isA(id, instr);
}

bool
isInstructionFieldMarker(InstructionField field);

bool
isBranchInstruction(InstructionID id);

const char *
getInstructionFieldName(InstructionField field);

uint32_t
getInstructionFieldStart(InstructionField field);

uint32_t
getInstructionFieldEnd(InstructionField field);

uint32_t
getInstructionFieldWidth(InstructionField field);

uint32_t
getInstructionFieldBitmask(InstructionField field);

} // namespace espresso
