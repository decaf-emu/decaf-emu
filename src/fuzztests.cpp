#include <random>
#include <string>
#include "fuzztests.h"
#include "instructionid.h"
#include "instructiondata.h"
#include "log.h"
#include "ppc.h"

template<size_t SIZE, class T> inline size_t array_size(T (&arr)[SIZE]) {
   return SIZE;
}

struct InstructionFuzzData {
   uint32_t baseInstr;
   std::vector<Field> allFields;
};

std::vector<InstructionFuzzData> instructionFuzzData;

bool buildFuzzData(InstructionID instrId, InstructionFuzzData &fuzzData)
{
   if (instrId == InstructionID::Invalid) {
      return true;
   }

   InstructionData *data = gInstructionTable.find(instrId);

   uint32_t instr = 0x00000000;
   uint32_t instrBits = 0x00000000;
   for (auto &op : data->opcode) {
      auto field = op.field;
      auto value = op.value;
      auto start = getFieldStart(field);
      if (start >= 32) continue;
      auto fieldBits = getFieldBitmask(field);
      if (instrBits & fieldBits) {
         gLog->error("Instruction {} opcode {} overwrites bits", data->name, (uint32_t)field);
         gLog->error("  {:032b} on {:032b}", fieldBits, instrBits);
         return false;
      }
      instrBits |= fieldBits;
      instr |= value << start;
   }

   std::vector<Field> allFields;

   for (auto i : data->read) {
      if (std::find(allFields.begin(), allFields.end(), i) == allFields.end()) {
         allFields.push_back(i);
      }
   }
   for (auto i : data->write) {
      if (std::find(allFields.begin(), allFields.end(), i) == allFields.end()) {
         allFields.push_back(i);
      }
   }
   for (auto i : data->flags) {
      if (std::find(allFields.begin(), allFields.end(), i) == allFields.end()) {
         allFields.push_back(i);
      }
   }

   for (auto i : allFields) {
      if (isFieldMarker(i)) continue;
      auto start = getFieldStart(i);
      auto end = getFieldEnd(i);
      auto fieldBits = getFieldBitmask(i);
      if (instrBits & fieldBits) {
         gLog->error("Instruction {} field {} overwrites bits", data->name, (uint32_t)i);
         gLog->error("  {:032b} on {:032b}", fieldBits, instrBits);
         return false;
      }
      instrBits |= fieldBits;
   }

   if (instrBits != 0xFFFFFFFF) {
      gLog->error("Instruction {} does not describe all its bits", data->name);
      gLog->error("  {:032b}", instrBits);
      return false;
   }

   fuzzData.baseInstr = instr;
   fuzzData.allFields = std::move(allFields);
   return true;
}

bool
setupFuzzData() {
   instructionFuzzData.resize((size_t)InstructionID::InstructionCount);
   bool res = true;
   for (int i = 0; i < (int)InstructionID::InstructionCount; ++i) {
      res &= buildFuzzData((InstructionID)i, instructionFuzzData[i]);
   }
   return res;
}

void setFieldValue(Instruction &instr, Field field, uint32_t value) {
   instr.value |= (value << getFieldStart(field)) & getFieldBitmask(field);
}

static void
encodeSPR(Instruction &instr, SprEncoding spr)
{
   uint32_t sprInt = (uint32_t)spr;
   instr.spr = ((sprInt << 5) & 0x3E0) | ((sprInt >> 5) & 0x1F);
}

bool
executeInstrTest(uint32_t test_seed, InstructionID instrId)
{
   // Special cases that we can't test easily
   switch (instrId) {
   case InstructionID::Invalid:
      return true;
   case InstructionID::lmw:
   case InstructionID::stmw:
   case InstructionID::stswi:
   case InstructionID::stswx:
      // Disabled for now
      return true;
   case InstructionID::b:
   case InstructionID::bc:
   case InstructionID::bcctr:
   case InstructionID::bclr:
      // Branching cannot be fuzzed
      return true;
   case InstructionID::kc:
      // Emulator Instruction
      return true;
   case InstructionID::sc:
   case InstructionID::tw:
   case InstructionID::twi:
   case InstructionID::mfsr:
   case InstructionID::mfsrin:
   case InstructionID::mtsr:
   case InstructionID::mtsrin:
      // Supervisory Instructions
      return true;
   }

   std::mt19937 test_rand(test_seed);
   const InstructionData *data = gInstructionTable.find(instrId);
   const InstructionFuzzData *fuzzData = &instructionFuzzData[(int)instrId];
   if (!data || !fuzzData) {
      return false;
   }

   Instruction instr(fuzzData->baseInstr);

   uint32_t gprAlloc = 5;
   uint32_t fprAlloc = 0;
   for (auto i : fuzzData->allFields) {
      if (isFieldMarker(i)) {
         continue;
      }

      switch (i) {
      case Field::frA: // gpr Targets
      case Field::frB:
      case Field::frC:
      case Field::frD:
      case Field::frS:
         setFieldValue(instr, i, fprAlloc++);
         break;
      case Field::rA: // fpr Targets
      case Field::rB:
      case Field::rD:
      case Field::rS:
         setFieldValue(instr, i, gprAlloc++);
         break;
      case Field::crbA: // crb Targets
      case Field::crbB:
      case Field::crbD:
         setFieldValue(instr, i, test_rand());
         break;
      case Field::crfD: // crf Targets
      case Field::crfS:
         setFieldValue(instr, i, test_rand());
         break;
      case Field::i: // gqr Targets
      case Field::qi:
         break;
      case Field::imm: // Random Values
      case Field::simm:
      case Field::uimm:
      case Field::rc: // Record Condition
      case Field::oe:
      case Field::crm:
      case Field::fm:
      case Field::w:
      case Field::qw:
      case Field::sh: // Shift Registers
      case Field::mb:
      case Field::me:
         break;
         setFieldValue(instr, i, test_rand());
         break;
      case Field::d: // Memory Delta...
      case Field::qd:
         setFieldValue(instr, i, test_rand());
         break;
      case Field::spr: // Special Purpose Registers
      {
         SprEncoding validSprs[] = {
            SprEncoding::XER,
            SprEncoding::LR,
            SprEncoding::CTR,
            SprEncoding::GQR0,
            SprEncoding::GQR1,
            SprEncoding::GQR2,
            SprEncoding::GQR3,
            SprEncoding::GQR4,
            SprEncoding::GQR5,
            SprEncoding::GQR6,
            SprEncoding::GQR7 };
         encodeSPR(instr, validSprs[test_rand() % array_size(validSprs)]);
         break;
      }
      case Field::tbr: // Time Base Registers
      {
         SprEncoding validTbrs[] = {
            SprEncoding::TBL,
            SprEncoding::TBU };
         encodeSPR(instr, validTbrs[test_rand() % array_size(validTbrs)]);
         break;
      }
      case Field::l:
         // l always must be 0
         instr.l = 0;
         break;

      default:
         gLog->error("Instruction {} field {} is unsupported by fuzzer", data->name, (uint32_t)i);
         return false;
      }
   }

   return true;
}

bool
executeFuzzTests(uint32_t suite_seed)
{
   if (!setupFuzzData()) {
      return false;
   }

   std::mt19937 suite_rand(suite_seed);
   for (auto i = 0; i < 100; ++i) {
      InstructionID instrId = (InstructionID)(i % (int)InstructionID::InstructionCount);
      executeInstrTest(suite_rand(), instrId);
   }

   return true;
}
