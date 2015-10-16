#include <random>
#include <string>
#include "fuzztests.h"
#include "cpu/instructionid.h"
#include "cpu/instructiondata.h"
#include "log.h"
#include "cpu/state.h"
#include "mem/mem.h"
#include "bitutils.h"
#include "trace.h"
#include "cpu/interpreter/interpreter.h"
#include "cpu/jit/jit.h"
#include "cpu/interpreter/interpreter_insreg.h"
#include "cpu/jit/jit_insreg.h"

template<size_t SIZE, class T> inline size_t array_size(T (&arr)[SIZE]) {
   return SIZE;
}

struct InstructionFuzzData {
   uint32_t baseInstr;
   std::vector<Field> allFields;
};

static const uint32_t instructionBase = 0x02000000;
static const uint32_t dataBase = 0x03000000;
std::vector<InstructionFuzzData> instructionFuzzData;

bool buildFuzzData(InstructionID instrId, InstructionFuzzData &fuzzData)
{
   if (instrId == InstructionID::Invalid) {
      return true;
   }

   InstructionData *data = gInstructionTable.find(instrId);

   // Verify JIT and Interpreter have everything registered...
   bool hasInterpHandler = cpu::interpreter::hasInstruction(static_cast<InstructionID>(instrId));
   bool hasJitHandler = cpu::jit::hasInstruction(static_cast<InstructionID>(instrId));
   if ((hasInterpHandler ^ hasJitHandler) != 0) {
      if (!hasInterpHandler) {
         gLog->error("Instruction {} has a JIT handler but no Interpreter handler", data->name);
      }
      if (!hasJitHandler) {
         gLog->error("Instruction {} has a Interpreter handler but no JIT handler", data->name);
      }
      return false;
   }

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
compareStateField(StateField::Field field, const TraceFieldValue &x, const TraceFieldValue &y, const TraceFieldValue &m, bool neg = false)
{
   if (field >= StateField::FPR0 && field <= StateField::FPR31) {
      uint64_t xa = x.u64v0 & m.u64v0;
      uint64_t xb = x.u64v1 & m.u64v1;
      uint64_t ya = y.u64v0 & m.u64v0;
      uint64_t yb = y.u64v1 & m.u64v1;
      return (*(double*)xa) == (*(double*)ya) && (*(double*)xb) == (*(double*)yb);
   }

   return (x.u32v0 & m.u32v0) == (y.u32v0 & m.u32v0) &&
      (x.u32v0 & m.u32v1) == (y.u32v0 & m.u32v1) &&
      (x.u32v0 & m.u32v2) == (y.u32v0 & m.u32v2) &&
      (x.u32v0 & m.u32v3) == (y.u32v0 & m.u32v3);
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

   if (!cpu::interpreter::hasInstruction(instrId)) {
      // No handler, skip it...
      return true;
   }

   Instruction instr(fuzzData->baseInstr);

   uint32_t gprAlloc = 5;
   uint32_t fprAlloc = 0;
   for (auto i : fuzzData->allFields) {
      if (isFieldMarker(i)) {
         continue;
      }

      switch (i) {
      case Field::rA: // gpr Targets
      case Field::rB:
      case Field::rD:
      case Field::rS:
         setFieldValue(instr, i, gprAlloc++);
         break;
      case Field::frA: // fpr Targets
      case Field::frB:
      case Field::frC:
      case Field::frD:
      case Field::frS:
         setFieldValue(instr, i, fprAlloc++);
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

   // Write an instruction
   mem::write(instructionBase + 0, instr.value);
  
   // Write a return for the Interpreter
   Instruction bclr = gInstructionTable.encode(InstructionID::bclr);
   bclr.bo = 0x1f;
   mem::write(instructionBase + 4, bclr.value);

   // Create States
   ThreadState stateFF, state00;
   memset(&state00, 0x00, sizeof(ThreadState));
   memset(&stateFF, 0xFF, sizeof(ThreadState));

   // Set up initial state
   TraceFieldValue writeMask[StateField::Max];
   memset(&writeMask, 0, sizeof(writeMask));

   for (auto i : data->write) {
      switch (i) {
      case Field::rA:
         writeMask[StateField::GPR + instr.rA].u32v0 = 0xFFFFFFFF; break;
      case Field::rB:
         writeMask[StateField::GPR + instr.rB].u32v0 = 0xFFFFFFFF; break;
      case Field::rD:
         writeMask[StateField::GPR + instr.rD].u32v0 = 0xFFFFFFFF; break;
      case Field::rS:
         writeMask[StateField::GPR + instr.rS].u32v0 = 0xFFFFFFFF; break;
      case Field::frA:
         writeMask[StateField::FPR + instr.frA].u64v0 = 0xFFFFFFFFFFFFFFFF;
         writeMask[StateField::FPR + instr.frA].u64v1 = 0xFFFFFFFFFFFFFFFF;
         break;
      case Field::frB:
         writeMask[StateField::FPR + instr.frB].u64v0 = 0xFFFFFFFFFFFFFFFF;
         writeMask[StateField::FPR + instr.frB].u64v1 = 0xFFFFFFFFFFFFFFFF;
         break;
      case Field::frC:
         writeMask[StateField::FPR + instr.frC].u64v0 = 0xFFFFFFFFFFFFFFFF;
         writeMask[StateField::FPR + instr.frC].u64v1 = 0xFFFFFFFFFFFFFFFF;
         break;
      case Field::frD:
         writeMask[StateField::FPR + instr.frD].u64v0 = 0xFFFFFFFFFFFFFFFF;
         writeMask[StateField::FPR + instr.frD].u64v1 = 0xFFFFFFFFFFFFFFFF;
         break;
      case Field::frS:
         writeMask[StateField::FPR + instr.frS].u64v0 = 0xFFFFFFFFFFFFFFFF;
         writeMask[StateField::FPR + instr.frS].u64v1 = 0xFFFFFFFFFFFFFFFF;
         break;
      case Field::i:
         writeMask[StateField::GQR + instr.i].u32v0 = 0xFFFFFFFF; break;
      case Field::qi:
         writeMask[StateField::GQR + instr.qi].u32v0 = 0xFFFFFFFF; break;
      case Field::crbA:
         writeMask[StateField::CR].u32v0 |= (1 << instr.crbA); break;
      case Field::crbB:
         writeMask[StateField::CR].u32v0 |= (1 << instr.crbA); break;
      case Field::crbD:
         writeMask[StateField::CR].u32v0 |= (1 << instr.crbA); break;
      case Field::crfS:
         writeMask[StateField::CR].u32v0 |= (0xF << instr.crfS); break;
      case Field::crfD:
         writeMask[StateField::CR].u32v0 |= (0xF << instr.crfD); break;
      case Field::RSRV:
         writeMask[StateField::ReserveAddress].u32v0 |= 0xFFFFFFFF; break;
      case Field::CTR:
         writeMask[StateField::CTR].u32v0 |= 0xFFFFFFFF; break;
         // TODO: The following fields should probably be
         //   more specific in the instruction data...
      case Field::CR:
         writeMask[StateField::CR].u32v0 |= 0xFFFFFFFF; break;
      case Field::FPSCR:
         writeMask[StateField::FPSCR].u32v0 |= 0xFFFFFFFF; break;
      case Field::XER:
         writeMask[StateField::XER].u32v0 |= 0xFFFFFFFF; break;
      default:
         assert(0);
      }
   }

   // Required to be set to this
   state00.tracer = nullptr;
   state00.cia = 0;
   state00.nia = instructionBase;
   stateFF.tracer = nullptr;
   stateFF.cia = 0;
   stateFF.nia = instructionBase;

   ThreadState iState00, iStateFF, jState00, jStateFF;
   memcpy(&iState00, &state00, sizeof(ThreadState));
   memcpy(&iStateFF, &stateFF, sizeof(ThreadState));
   memcpy(&jState00, &state00, sizeof(ThreadState));
   memcpy(&jStateFF, &stateFF, sizeof(ThreadState));

   {
      cpu::interpreter::executeSub(&iState00);
      cpu::interpreter::executeSub(&iStateFF);
   }

   {
      cpu::jit::clearCache();
      cpu::jit::executeSub(&jState00);
      cpu::jit::executeSub(&jStateFF);

      // Make sure CIA/NIA states match...
      jState00.cia = iState00.cia;
      jState00.nia = iState00.nia;
      jStateFF.cia = iStateFF.cia;
      jStateFF.nia = iStateFF.nia;
   }

   // Check that we have no unexpected writes
   // Compare iState00 vs iStateFF
   // Compare jState00 vs jStateFF

   for (auto i = 0; i < StateField::Max; ++i) {
      if (i == StateField::Invalid) {
         continue;
      }

      TraceFieldValue val00, valFF;
      saveStateField(&iState00, i, val00);
      saveStateField(&iStateFF, i, valFF);
   }

   // Check that we have no unexpected reads
   // Compare iState00 vs state00
   // Compare jState00 vs state00

   // Compare iState00 and jState00 to hardware...


   return true;
}

bool
executeFuzzTests(uint32_t suite_seed)
{
   if (!setupFuzzData()) {
      return false;
   }

   mem::alloc(instructionBase, 32);
   mem::alloc(dataBase, 128);

   std::mt19937 suite_rand(suite_seed);
   for (auto i = 0; i < 100; ++i) {
      InstructionID instrId = (InstructionID)(i % (int)InstructionID::InstructionCount);
      executeInstrTest(suite_rand(), instrId);
   }

   return true;
}
