#pragma once
#include <cstdint>
#include <vector>
#include "instruction.h"
#include "instructionid.h"

// TODO: Move
struct BitRange
{
   int start;
   int end;
};

enum class Field
{
   Invalid,
   aa,   // 30
   bd,   // 16-29
   bi,   // 11-15
   bo,   // 6-10
   crbA, // 11-15
   crbB, // 16-20
   crbD, // 6-10
   crfD, // 6-8
   crfS, // 11-13
   crm,  // 12-19
   d,    // 16-31 or 20-31...
   fm,   // 7-14
   frA,  // 11-15
   frB,  // 16-20
   frC,  // 21-25
   frD,  // 6-10
   frS,  // 6-10
   i,    // 17-19
   imm,  // 16-19
   l,    // 10
   li,   // 6-29
   lk,   // 31
   mb,   // 21-25
   me,   // 26-30
   nb,   // 16-20
   oe,   // 21
   opcd, // 0-5
   qi,   // 22-24
   qw,   // 21
   rA,   // 11-15
   rB,   // 16-20
   rc,   // 31
   rD,   // 6-10
   rS,   // 6-10
   sh,   // 16-20
   simm, // 16-31
   sr,   // 12-15
   spr,  // 11-20
   to,   // 6-10
   tbr,  // 11-20
   uimm, // 16-31
   w,    // 16-16
   xo1,  // 21-30
   xo2,  // 22-30
   xo3,  // 25-30
   xo4,  // 26-30
};

struct InstructionData
{
   struct Opcode
   {
      Opcode()
      {
      }

      Opcode(Field field, uint32_t value) : field(field), value(value)
      {
      }

      Field field = Field::Invalid;
      uint32_t value = 0;
   };

   InstructionID id;
   const char *name;
   const char *fullname;
   std::vector<Opcode> opcode;
   std::vector<Field> read;
   std::vector<Field> write;
   std::vector<Field> flags;
};

uint32_t getFieldStart(Field field);
uint32_t getFieldEnd(Field field);
uint32_t getFieldWidth(Field field);
uint32_t getFieldBitmask(Field field);

struct InstructionTable
{
   void initialise();
   InstructionData *decode(Instruction instr);
   Instruction encode(InstructionID id);
};

extern InstructionTable gInstructionTable;
