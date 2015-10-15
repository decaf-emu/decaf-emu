#include <random>
#include <string>
#include "fuzztests.h"
#include "instructionid.h"
#include "instructiondata.h"
#include "instructiondata.h"
#include "log.h"

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
      return true;
   }

   std::mt19937 test_rand(test_seed);
   const InstructionData *data = gInstructionTable.find(instrId);
   const InstructionFuzzData *fuzzData = &instructionFuzzData[(int)instrId];



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
