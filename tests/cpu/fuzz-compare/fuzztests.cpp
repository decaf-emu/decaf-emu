#include <algorithm>
#include <random>
#include <string>
#include "fuzztests.h"
#include "common/bitutils.h"
#include "common/log.h"
#include "libcpu/src/interpreter/interpreter.h"
#include "libcpu/src/interpreter/interpreter_insreg.h"
#include "libcpu/src/jit/jit.h"
#include "libcpu/state.h"
#include "libcpu/src/utils.h"
#include "libcpu/mem.h"
#include "libcpu/trace.h"
#include "libcpu/espresso/espresso_instructionset.h"
#include "libcpu/espresso/espresso_spr.h"
using namespace espresso;

template<size_t SIZE, class T> inline size_t array_size(T (&arr)[SIZE]) {
   return SIZE;
}

struct InstructionFuzzData {
   uint32_t baseInstr;
   std::vector<InstructionField> allFields;
};

static const uint32_t instructionBase = mem::MEM2Base;
static const uint32_t dataBase = instructionBase + 0x01000000;
std::vector<InstructionFuzzData> instructionFuzzData;

bool buildFuzzData(InstructionID instrId, InstructionFuzzData &fuzzData)
{
   if (instrId == InstructionID::Invalid) {
      return true;
   }

   auto data = findInstructionInfo(instrId);

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
      auto start = getInstructionFieldStart(field);
      if (start >= 32) continue;
      auto fieldBits = getInstructionFieldBitmask(field);
      if (instrBits & fieldBits) {
         gLog->error("Instruction {} opcode {} overwrites bits", data->name, (uint32_t)field);
         gLog->error("  {:032b} on {:032b}", fieldBits, instrBits);
         return false;
      }
      instrBits |= fieldBits;
      instr |= value << start;
   }

   std::vector<InstructionField> allFields;

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
      if (isInstructionFieldMarker(i)) continue;
      auto fieldBits = getInstructionFieldBitmask(i);
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

void setFieldValue(Instruction &instr, InstructionField field, uint32_t value) {
   instr.value |= (value << getInstructionFieldStart(field)) & getInstructionFieldBitmask(field);
}

bool
compareStateField(int field, const TraceFieldValue &x, const TraceFieldValue &y)
{
   if (field >= StateField::FPR0 && field <= StateField::FPR31) {
      return fabs(x.f64v0 - y.f64v0) < 0.0001 && fabs(x.f64v1 - y.f64v1) < 0.0001;
   }
   return x.u64v0 == y.u64v0 && x.u64v1 == y.u64v1;
}

bool
compareStateField(int field, const TraceFieldValue &x, const TraceFieldValue &y, const TraceFieldValue &m, bool neg = false)
{
   if (field >= StateField::FPR0 && field <= StateField::FPR31) {
      uint64_t xa = x.u64v0 & m.u64v0;
      uint64_t xb = x.u64v1 & m.u64v1;
      uint64_t ya = y.u64v0 & m.u64v0;
      uint64_t yb = y.u64v1 & m.u64v1;
      return (*(double*)&xa) == (*(double*)&ya) && (*(double*)&xb) == (*(double*)&yb);
   }

   return (x.u32v0 & m.u32v0) == (y.u32v0 & m.u32v0) &&
      (x.u32v0 & m.u32v1) == (y.u32v0 & m.u32v1) &&
      (x.u32v0 & m.u32v2) == (y.u32v0 & m.u32v2) &&
      (x.u32v0 & m.u32v3) == (y.u32v0 & m.u32v3);
}

bool
executeInstrTest(uint32_t test_seed)
{
   std::mt19937 test_rand(test_seed);
   InstructionID instrId = (InstructionID)(test_rand() % (int)InstructionID::InstructionCount);

   // Special cases that we can't test easily
   switch (instrId) {
   case InstructionID::Invalid:
      return true;
   case InstructionID::lmw:
   case InstructionID::lswi:
   case InstructionID::lswx:
   case InstructionID::stmw:
   case InstructionID::stswi:
   case InstructionID::stswx:
      // Multi-word logic, disabled for now
      return true;
   case InstructionID::psq_l:
   case InstructionID::psq_lu:
   case InstructionID::psq_lux:
   case InstructionID::psq_lx:
   case InstructionID::psq_st:
   case InstructionID::psq_stu:
   case InstructionID::psq_stux:
   case InstructionID::psq_stx:
      // Quantization Registers need to be properly configured for these, disabled for now
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
   default:
      break;
   }

   const auto data = findInstructionInfo(instrId);
   const InstructionFuzzData *fuzzData = &instructionFuzzData[(int)instrId];
   if (!data || !fuzzData) {
      return false;
   }

   if (!cpu::interpreter::hasInstruction(instrId)) {
      // No handler, skip it...
      return true;
   }

   Instruction instr(fuzzData->baseInstr);

   {
      // TODO: Add handling for rA==0 being 0 :S
      uint32_t gprAlloc = 0;
      uint32_t fprAlloc = 0;
      uint32_t gqrAlloc = 0;
      static const uint32_t gprAllocatable[] = { 5, 6, 7, 8, 9 };
      static const uint32_t fprAllocatable[] = { 0, 1, 2, 3 };
      static const uint32_t gqrAllocatable[] = { 0, 1, 2, 3 };
      auto nextGpr = [&]() {
         assert(gprAlloc < array_size(gprAllocatable));
         return gprAllocatable[gprAlloc++];
      };
      auto nextFpr = [&]() {
         assert(fprAlloc < array_size(fprAllocatable));
         return fprAllocatable[fprAlloc++];
      };
      auto nextGqr = [&]() {
         assert(gqrAlloc < array_size(gqrAllocatable));
         return gqrAllocatable[gqrAlloc++];
      };
      for (auto i : fuzzData->allFields) {
         if (isInstructionFieldMarker(i)) {
            continue;
         }

         switch (i) {
         case InstructionField::rA: // gpr Targets
         case InstructionField::rB:
         case InstructionField::rD:
         case InstructionField::rS:
            setFieldValue(instr, i, nextGpr());
            break;
         case InstructionField::frA: // fpr Targets
         case InstructionField::frB:
         case InstructionField::frC:
         case InstructionField::frD:
         case InstructionField::frS:
            setFieldValue(instr, i, nextFpr());
            break;
         case InstructionField::i: // gqr Targets
         case InstructionField::qi:
            setFieldValue(instr, i, test_rand() & 4);
         case InstructionField::crbA: // crb Targets
         case InstructionField::crbB:
         case InstructionField::crbD:
            setFieldValue(instr, i, test_rand());
            break;
         case InstructionField::crfD: // crf Targets
         case InstructionField::crfS:
            setFieldValue(instr, i, test_rand());
            break;
         case InstructionField::imm: // Random Values
         case InstructionField::simm:
         case InstructionField::uimm:
         case InstructionField::rc: // Record Condition
         case InstructionField::frc:
         case InstructionField::oe:
         case InstructionField::crm:
         case InstructionField::fm:
         case InstructionField::w:
         case InstructionField::qw:
         case InstructionField::sh: // Shift Registers
         case InstructionField::mb:
         case InstructionField::me:
            setFieldValue(instr, i, test_rand());
            break;
         case InstructionField::d: // Memory Delta...
         case InstructionField::qd:
            setFieldValue(instr, i, test_rand());
            break;
         case InstructionField::spr: // Special Purpose Registers
         {
            SPR validSprs[] = {
               SPR::XER,
               SPR::CTR,
               SPR::GQR0,
               SPR::GQR1,
               SPR::GQR2,
               SPR::GQR3,
               SPR::GQR4,
               SPR::GQR5,
               SPR::GQR6,
               SPR::GQR7
            };
            encodeSPR(instr, validSprs[test_rand() % array_size(validSprs)]);
            break;
         }
         case InstructionField::tbr: // Time Base Registers
         {
            SPR validTbrs[] = {
               SPR::TBL,
               SPR::TBU
            };
            encodeSPR(instr, validTbrs[test_rand() % array_size(validTbrs)]);
            break;
         }
         case InstructionField::l:
            // l always must be 0
            instr.l = 0;
            break;

         default:
            gLog->error("Instruction {} field {} is unsupported by fuzzer", data->name, (uint32_t)i);
            return false;
         }
      }
   }

   // Write an instruction
   mem::write(instructionBase + 0, instr.value);

   // Write a return for the Interpreter
   auto bclr = encodeInstruction(InstructionID::bclr);
   bclr.bo = 0x1f;
   mem::write(instructionBase + 4, bclr.value);

#define STATEFIELDO(x, y) (StateField::Field)((int)x + y)
   StateField::Field randFields[] = {
      STATEFIELDO(StateField::GPR, 0),
      STATEFIELDO(StateField::GPR, 5),
      STATEFIELDO(StateField::GPR, 6),
      STATEFIELDO(StateField::GPR, 7),
      STATEFIELDO(StateField::GPR, 8),
      STATEFIELDO(StateField::GPR, 9),
      STATEFIELDO(StateField::FPR, 0),
      STATEFIELDO(StateField::FPR, 1),
      STATEFIELDO(StateField::FPR, 2),
      STATEFIELDO(StateField::FPR, 3),
      STATEFIELDO(StateField::GQR, 0),
      STATEFIELDO(StateField::GQR, 1),
      STATEFIELDO(StateField::GQR, 2),
      STATEFIELDO(StateField::GQR, 3),
      StateField::CR,
      StateField::FPSCR,
      StateField::XER,
      StateField::CTR
   };
#undef STATEFIELDO
   size_t numRandFields = array_size(randFields);

   // Build some randomized state data
   cpu::Core iState, jState;
   for (auto i = 0u; i < numRandFields; ++i) {
      auto field = randFields[i];

      TraceFieldValue v;
      v.u32v0 = test_rand();
      v.u32v1 = test_rand();
      v.u32v2 = test_rand();
      v.u32v3 = test_rand();

      restoreStateField(&iState, field, v);
      restoreStateField(&jState, field, v);
   }

   // Build some randomized memory data
   const uint32_t memSize = 64;
   uint8_t iMem[memSize], jMem[memSize];
   for (auto i = 0u; i < memSize; ++i) {
      auto randVal = (uint8_t)test_rand();
      iMem[i] = randVal;
      jMem[i] = randVal;
   }

   // Some instructions need to be forced to a certain address
#define SETGPR(i, v) \
   iState.gpr[i]=v; \
   jState.gpr[i]=v;
#define CONFIG_rA_rB() { \
      auto d = static_cast<int32_t>(test_rand()); \
      SETGPR(instr.rA, d); \
      SETGPR(instr.rB, dataBase - d); \
      break; }
#define CONFIG_rA_D() { \
      auto d = sign_extend<16, int32_t>(instr.d); \
      SETGPR(instr.rA, dataBase - d); \
      break; }
#define CONFIG_rA_QD() { \
      auto d = sign_extend<12, int32_t>(instr.qd); \
      SETGPR(instr.rA, dataBase - d); \
      break; }

   switch (instrId) {
   case InstructionID::lbz: CONFIG_rA_D();
   case InstructionID::lbzu: CONFIG_rA_D();
   case InstructionID::lha: CONFIG_rA_D();
   case InstructionID::lhau: CONFIG_rA_D();
   case InstructionID::lhz: CONFIG_rA_D();
   case InstructionID::lhzu: CONFIG_rA_D();
   case InstructionID::lwz: CONFIG_rA_D();
   case InstructionID::lwzu: CONFIG_rA_D();
   case InstructionID::lfs: CONFIG_rA_D();
   case InstructionID::lfsu: CONFIG_rA_D();
   case InstructionID::lfd: CONFIG_rA_D();
   case InstructionID::lfdu: CONFIG_rA_D();
   case InstructionID::lbzx: CONFIG_rA_rB();
   case InstructionID::lbzux: CONFIG_rA_rB();
   case InstructionID::lhax: CONFIG_rA_rB();
   case InstructionID::lhaux: CONFIG_rA_rB();
   case InstructionID::lhbrx: CONFIG_rA_rB();
   case InstructionID::lhzx: CONFIG_rA_rB();
   case InstructionID::lhzux: CONFIG_rA_rB();
   case InstructionID::lwbrx: CONFIG_rA_rB();
   case InstructionID::lwarx: CONFIG_rA_rB();
   case InstructionID::lwzx: CONFIG_rA_rB();
   case InstructionID::lwzux: CONFIG_rA_rB();
   case InstructionID::lfsx: CONFIG_rA_rB();
   case InstructionID::lfsux: CONFIG_rA_rB();
   case InstructionID::lfdx: CONFIG_rA_rB();
   case InstructionID::lfdux: CONFIG_rA_rB();

   case InstructionID::stb: CONFIG_rA_D();
   case InstructionID::stbu: CONFIG_rA_D();
   case InstructionID::sth: CONFIG_rA_D();
   case InstructionID::sthu: CONFIG_rA_D();
   case InstructionID::stw: CONFIG_rA_D();
   case InstructionID::stwu: CONFIG_rA_D();
   case InstructionID::stfs: CONFIG_rA_D();
   case InstructionID::stfsu: CONFIG_rA_D();
   case InstructionID::stfd: CONFIG_rA_D();
   case InstructionID::stfdu: CONFIG_rA_D();
   case InstructionID::stbx: CONFIG_rA_rB();
   case InstructionID::stbux: CONFIG_rA_rB();
   case InstructionID::sthx: CONFIG_rA_rB();
   case InstructionID::sthux: CONFIG_rA_rB();
   case InstructionID::stwx: CONFIG_rA_rB();
   case InstructionID::stwux: CONFIG_rA_rB();
   case InstructionID::sthbrx: CONFIG_rA_rB();
   case InstructionID::stwbrx: CONFIG_rA_rB();
   case InstructionID::stwcx: CONFIG_rA_rB();
   case InstructionID::stfsx: CONFIG_rA_rB();
   case InstructionID::stfsux: CONFIG_rA_rB();
   case InstructionID::stfdx: CONFIG_rA_rB();
   case InstructionID::stfdux: CONFIG_rA_rB();
   case InstructionID::stfiwx: CONFIG_rA_rB();

   case InstructionID::psq_l: CONFIG_rA_QD();
   case InstructionID::psq_lx: CONFIG_rA_rB();
   case InstructionID::psq_lu: CONFIG_rA_QD();
   case InstructionID::psq_lux: CONFIG_rA_rB();

   case InstructionID::dcbz: CONFIG_rA_rB();
   case InstructionID::dcbz_l: CONFIG_rA_rB();

   default:
      break;
   }

#undef SETGPR
#undef CONFIG_rA_rB
#undef CONFIG_rA_D
#undef CONFIG_rA_QD


   // Disable Reserveds for now
   iState.reserve = false;
   jState.reserve = false;

   // Required to be set to this
   iState.tracer = nullptr;
   iState.cia = 0;
   iState.nia = instructionBase;
   jState.tracer = nullptr;
   jState.cia = 0;
   jState.nia = instructionBase;

   {
      memcpy(mem::translate(dataBase), iMem, memSize);
      cpu::interpreter::executeSub(&iState);
      memcpy(iMem, mem::translate(dataBase), memSize);
   }

   {
      cpu::jit::clearCache();

      memcpy(mem::translate(dataBase), jMem, memSize);
      cpu::jit::executeSub(&jState);
      memcpy(jMem, mem::translate(dataBase), memSize);
   }

   for (auto i = 0u; i < numRandFields; ++i) {
      auto field = randFields[i];

      TraceFieldValue iVal, jVal;
      saveStateField(&iState, field, iVal);
      saveStateField(&jState, field, jVal);

      if (!compareStateField(field, iVal, jVal)) {
         gLog->warn("{}({:08x}) :: JIT does not match Interp on {}", data->name, test_seed, getStateFieldName(field));
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

   for (auto i = 0; i < 10000; ++i) {
      if (false) {
         gLog->info("Executing test {}", i);
      }

      executeInstrTest(suite_rand());
   }

   return true;
}
