#include <random>
#include <string>
#include "fuzztests.h"
#include "cpu/instructionid.h"
#include "cpu/instructiondata.h"
#include "cpu/utils.h"
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
      case Field::frc:
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

   // Create memory values
   static const size_t memSize = 16;
   uint8_t memFF[memSize], mem00[memSize], memMask[memSize];
   memset(mem00, 0x01, memSize);
   memset(memFF, 0xFE, memSize);
   memset(memMask, 0x00, memSize);

   std::vector<Field> insReads = data->read;
   std::vector<Field> insWrites = data->write;

   bool isPairedSinglesInstr = false;
   for (auto i : data->flags) {
      switch (i) {
         switch (i) {
            // Flag Types
         case Field::rc:
            if (!instr.rc) {
               break;
            }
            // Falls Through
         case Field::ARC:
            insReads.push_back(Field::XERSO);
            insWrites.push_back(Field::CR0);
            break;
         case Field::frc:
            if (!instr.rc) {
               break;
            }
            insWrites.push_back(Field::CR1);
         case Field::oe:
            if (!instr.oe) {
               break;
            }
            // Falls Through...
         case Field::AOE:
            insReads.push_back(Field::XERO);
            break;
         case Field::PS:
            isPairedSinglesInstr;
            break;
         default:
            assert(0);
         }

      }
   }


   // Set up initial state
   TraceFieldValue writeMask[StateField::Max];
   memset(&writeMask, 0, sizeof(writeMask));

   for (auto i : insWrites) {
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
         if (isPairedSinglesInstr) {
            writeMask[StateField::FPR + instr.frA].u64v1 = 0xFFFFFFFFFFFFFFFF;
         }
         break;
      case Field::frB:
         writeMask[StateField::FPR + instr.frB].u64v0 = 0xFFFFFFFFFFFFFFFF;
         if (isPairedSinglesInstr) {
            writeMask[StateField::FPR + instr.frB].u64v1 = 0xFFFFFFFFFFFFFFFF;
         }
         break;
      case Field::frC:
         writeMask[StateField::FPR + instr.frC].u64v0 = 0xFFFFFFFFFFFFFFFF;
         if (isPairedSinglesInstr) {
            writeMask[StateField::FPR + instr.frC].u64v1 = 0xFFFFFFFFFFFFFFFF;
         }
         break;
      case Field::frD:
         writeMask[StateField::FPR + instr.frD].u64v0 = 0xFFFFFFFFFFFFFFFF;
         if (isPairedSinglesInstr) {
            writeMask[StateField::FPR + instr.frD].u64v1 = 0xFFFFFFFFFFFFFFFF;
         }
         break;
      case Field::frS:
         writeMask[StateField::FPR + instr.frS].u64v0 = 0xFFFFFFFFFFFFFFFF;
         if (isPairedSinglesInstr) {
            writeMask[StateField::FPR + instr.frS].u64v1 = 0xFFFFFFFFFFFFFFFF;
         }
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
         writeMask[StateField::CR].u32v0 |= (0xF << ((7 - instr.crfS) * 4)); break;
      case Field::crfD:
         writeMask[StateField::CR].u32v0 |= (0xF << ((7 - instr.crfD) * 4)); break;
      case Field::RSRV:
         writeMask[StateField::ReserveAddress].u32v0 |= 0xFFFFFFFF; break;
      case Field::CTR:
         writeMask[StateField::CTR].u32v0 |= 0xFFFFFFFF; break;
      case Field::FPSCR:
         // DISABLED - SHOULDNT BE USED...
         break;
      case Field::FCRSNAN:
         writeMask[StateField::FPSCR].u32v0 |= FPSCRRegisterBits::VXSNAN; break;
      case Field::FCRISI:
         writeMask[StateField::FPSCR].u32v0 |= FPSCRRegisterBits::VXISI; break;
      case Field::FCRIDI:
         writeMask[StateField::FPSCR].u32v0 |= FPSCRRegisterBits::VXIDI; break;
      case Field::FCRZDZ:
         writeMask[StateField::FPSCR].u32v0 |= FPSCRRegisterBits::VXZDZ; break;
      case Field::XERO:
         writeMask[StateField::XER].u32v0 |= (XERegisterBits::Overflow | XERegisterBits::StickyOV); break;
      case Field::XERC:
         writeMask[StateField::XER].u32v0 |= XERegisterBits::Carry; break;
      case Field::CR0:
         writeMask[StateField::CR].u32v0 |= 0xF0000000; break;
      case Field::CR1:
         writeMask[StateField::CR].u32v0 |= 0x0F000000; break;
      default:
         assert(0);
      }
   }

#define GPRRAND(i) { \
   state00.gpr[i] = test_rand(); \
   stateFF.gpr[i] = state00.gpr[i]; \
   break; }
#define FPRRAND(i) { \
   state00.fpr[i].idw = test_rand(); \
   if (isPairedSinglesInstr) state00.fpr[i].idw1 = test_rand(); \
   stateFF.fpr[i].idw = state00.fpr[i].idw; \
   if (isPairedSinglesInstr) stateFF.fpr[i].idw1 = state00.fpr[i].idw1; \
   break; }
#define GQRRAND(i) { \
   state00.gqr[i].value = test_rand(); \
   stateFF.gqr[i].value = state00.gqr[i].value; \
   break; }
#define CRBRAND(i) { \
   auto randBit = test_rand() & 1; \
   setCRB(&state00, i, randBit); \
   setCRB(&stateFF, i, randBit); \
   break; }
#define CRFRAND(i) { \
   auto randBits = test_rand() & 0xF; \
   setCRF(&state00, i, randBits); \
   setCRF(&stateFF, i, randBits); \
   break; }
#define GENERICRAND(field) { \
   auto randVal = test_rand(); \
   state00.field = randVal; \
   stateFF.field = state00.field; \
   break; }

   for (auto i : insReads) {
      switch (i) {

         // State Related
      case Field::rA: GPRRAND(instr.rA);
      case Field::rB: GPRRAND(instr.rB);
      case Field::rD: GPRRAND(instr.rD);
      case Field::rS: GPRRAND(instr.rS);
      case Field::frA: FPRRAND(instr.frA);
      case Field::frB: FPRRAND(instr.frB);
      case Field::frC: FPRRAND(instr.frC);
      case Field::frD: FPRRAND(instr.frD);
      case Field::frS: FPRRAND(instr.frS);
      case Field::i: GQRRAND(instr.i);
      case Field::qi: GQRRAND(instr.qi);
      case Field::crbA: CRBRAND(instr.crbA);
      case Field::crbB: CRBRAND(instr.crbB);
      case Field::crbD: CRBRAND(instr.crbD);
      case Field::crfS: CRFRAND(instr.crfS);
      case Field::crfD: CRFRAND(instr.crfD);

         // Markers
      case Field::CTR: GENERICRAND(ctr);
      case Field::XERC: GENERICRAND(xer.ca);
      case Field::XERSO: GENERICRAND(xer.so);

         // Instruction Related
      case Field::imm:
      case Field::simm:
      case Field::uimm:
      case Field::crm:
      case Field::fm:
      case Field::w:
      case Field::qw:
      case Field::sh:
      case Field::mb:
      case Field::me:
      case Field::d:
      case Field::qd:
      case Field::spr:
      case Field::tbr:
      case Field::l:
         break;

         // Flag Types
      case Field::rc:
      case Field::frc:
      case Field::oe:
      case Field::ARC:
      case Field::AOE:
         assert(0);
         break;

      default:
         assert(0);
      }
   }

#undef GPRRAND
#undef FPRRAND
#undef CRBRAND
#undef CRFRAND
#undef GENERICRAND

   // Write random data to memory...
#define SETGPR(i, v) \
   state00.gpr[i]=v; \
   stateFF.gpr[i]=v;
#define MEMRAND(pos, size) \
   assert(pos + size < memSize); \
   for(auto i = 0; i < size; ++i) { \
      mem00[pos+i] = test_rand(); \
      memFF[pos+i] = test_rand(); \
   }
#define MEMMASK(pos, size) \
   assert(pos + size < memSize); \
   memset(memMask, 0xFF, size)
#define CONFIGDELTALOAD(Type) { \
      auto d = sign_extend<16, int32_t>(instr.d); \
      SETGPR(instr.rA, dataBase - d); \
      MEMRAND(0, sizeof(Type)) \
      break; }
#define CONFIGIDXDLOAD(Type) { \
      auto d = static_cast<int32_t>(test_rand()); \
      SETGPR(instr.rA, d); \
      SETGPR(instr.rB, dataBase - d); \
      MEMRAND(0, sizeof(Type)) \
      break; }
#define CONFIGDELTASTORE(Type) { \
      auto d = sign_extend<16, int32_t>(instr.d); \
      SETGPR(instr.rA, dataBase - d); \
      MEMMASK(0, sizeof(Type)); \
      break; }
#define CONFIGIDXDSTORE(Type) { \
      auto d = static_cast<int32_t>(test_rand()); \
      SETGPR(instr.rA, d); \
      SETGPR(instr.rB, dataBase - d); \
      MEMMASK(0, sizeof(Type)); \
      break; }

   switch (instrId) {
   case InstructionID::lbz: CONFIGDELTALOAD(uint8_t);
   case InstructionID::lbzu: CONFIGDELTALOAD(uint8_t);
   case InstructionID::lha: CONFIGDELTALOAD(uint16_t);
   case InstructionID::lhau: CONFIGDELTALOAD(uint16_t);
   case InstructionID::lhz: CONFIGDELTALOAD(uint16_t);
   case InstructionID::lhzu: CONFIGDELTALOAD(uint16_t);
   case InstructionID::lwz: CONFIGDELTALOAD(uint32_t);
   case InstructionID::lwzu: CONFIGDELTALOAD(uint32_t);
   case InstructionID::lfs: CONFIGDELTALOAD(float);
   case InstructionID::lfsu: CONFIGDELTALOAD(float);
   case InstructionID::lfd: CONFIGDELTALOAD(double);
   case InstructionID::lfdu: CONFIGDELTALOAD(double);
   case InstructionID::lbzx: CONFIGIDXDLOAD(uint8_t);
   case InstructionID::lbzux: CONFIGIDXDLOAD(uint8_t);
   case InstructionID::lhax: CONFIGIDXDLOAD(uint16_t);
   case InstructionID::lhaux: CONFIGIDXDLOAD(uint16_t);
   case InstructionID::lhbrx: CONFIGIDXDLOAD(uint16_t);
   case InstructionID::lhzx: CONFIGIDXDLOAD(uint16_t);
   case InstructionID::lhzux: CONFIGIDXDLOAD(uint16_t);
   case InstructionID::lwbrx: CONFIGIDXDLOAD(uint32_t);
   case InstructionID::lwarx: CONFIGIDXDLOAD(uint32_t);
   case InstructionID::lwzx: CONFIGIDXDLOAD(uint32_t);
   case InstructionID::lwzux: CONFIGIDXDLOAD(uint32_t);
   case InstructionID::lfsx: CONFIGIDXDLOAD(float);
   case InstructionID::lfsux: CONFIGIDXDLOAD(float);
   case InstructionID::lfdx: CONFIGIDXDLOAD(double);
   case InstructionID::lfdux: CONFIGIDXDLOAD(double);
   case InstructionID::stb: CONFIGDELTASTORE(uint8_t);
   case InstructionID::stbu: CONFIGDELTASTORE(uint8_t);
   case InstructionID::sth: CONFIGDELTASTORE(uint16_t);
   case InstructionID::sthu: CONFIGDELTASTORE(uint16_t);
   case InstructionID::stw: CONFIGDELTASTORE(uint32_t);
   case InstructionID::stwu: CONFIGDELTASTORE(uint32_t);
   case InstructionID::stfs: CONFIGDELTASTORE(float);
   case InstructionID::stfsu: CONFIGDELTASTORE(float);
   case InstructionID::stfd: CONFIGDELTASTORE(double);
   case InstructionID::stfdu: CONFIGDELTASTORE(double);
   case InstructionID::stbx: CONFIGIDXDSTORE(uint8_t);
   case InstructionID::stbux: CONFIGIDXDSTORE(uint8_t);
   case InstructionID::sthx: CONFIGIDXDSTORE(uint16_t);
   case InstructionID::sthux: CONFIGIDXDSTORE(uint16_t);
   case InstructionID::stwx: CONFIGIDXDSTORE(uint32_t);
   case InstructionID::stwux: CONFIGIDXDSTORE(uint32_t);
   case InstructionID::sthbrx: CONFIGIDXDSTORE(uint16_t);
   case InstructionID::stwbrx: CONFIGIDXDSTORE(uint32_t);
   case InstructionID::stwcx: CONFIGIDXDSTORE(uint32_t);
   case InstructionID::stfsx: CONFIGIDXDSTORE(float);
   case InstructionID::stfsux: CONFIGIDXDSTORE(float);
   case InstructionID::stfdx: CONFIGIDXDSTORE(double);
   case InstructionID::stfdux: CONFIGIDXDSTORE(double);
   case InstructionID::stfiwx: CONFIGIDXDSTORE(uint32_t);

   default:
      break;
   }

#undef SETGPR
#undef MEMRAND
#undef MEMMASK
#undef CONFIGDELTALOAD
#undef CONFIGIDXDLOAD
#undef CONFIGDELTASTORE
#undef CONFIGIDXDSTORE

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
   uint8_t jMem00[memSize], jMemFF[memSize], iMem00[memSize], iMemFF[memSize];

   {
      memcpy(mem::translate(dataBase), mem00, memSize);
      cpu::interpreter::executeSub(&iState00);
      memcpy(iMem00, mem::translate(dataBase), memSize);

      memcpy(mem::translate(dataBase), memFF, memSize);
      cpu::interpreter::executeSub(&iStateFF);
      memcpy(iMemFF, mem::translate(dataBase), memSize);
   }

   {
      cpu::jit::clearCache();

      memcpy(mem::translate(dataBase), mem00, memSize);
      cpu::jit::executeSub(&jState00);
      memcpy(jMem00, mem::translate(dataBase), memSize);

      memcpy(mem::translate(dataBase), memFF, memSize);
      cpu::jit::executeSub(&jStateFF);
      memcpy(jMemFF, mem::translate(dataBase), memSize);

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
      StateField::Field field = (StateField::Field)i;
      if (field == StateField::Invalid) {
         continue;
      }

      TraceFieldValue val00, valFF;
      saveStateField(&state00, field, val00);
      saveStateField(&stateFF, field, valFF);

      TraceFieldValue iVal00, iValFF, jVal00, jValFF;
      saveStateField(&iState00, field, iVal00);
      saveStateField(&iStateFF, field, iValFF);
      saveStateField(&jState00, field, jVal00);
      saveStateField(&jStateFF, field, jValFF);

      // -- Check for things that changed between 00 and FF
      // jMem
      if (!compareStateField(field, jVal00, jValFF, writeMask[field], false)) {
         gLog->warn("{} JIT {} behaviour affected by unexpected state", data->name, getStateFieldName(field));
      }

      // jMem
      if (!compareStateField(field, iVal00, iValFF, writeMask[field], false)) {
         gLog->warn("{} Interpreter {} behaviour affected by unexpected state", data->name, getStateFieldName(field));
      }
   }


   for (auto i = 0; i < memSize; ++i) {
      // -- Check for things that changed between 00 and FF
      // jMem
      if ((jMem00[i] & memMask[i]) != (jMemFF[i] & memMask[i])) {
         gLog->warn("JIT memory behaviour affected by unexpected state");
      }

      // iMem
      if ((iMem00[i] & memMask[i]) != (iMemFF[i] & memMask[i])) {
         gLog->warn("Interpreter memory behaviour affected by unexpected state");
      }


      // -- Check that nothing changed that shouldnt have
      // iMem
      if ((iMem00[i] & ~memMask[i]) != (mem00[i] & ~memMask[i]) ||
         (iMemFF[i] & ~memMask[i]) != (memFF[i] & ~memMask[i])) {
         gLog->warn("Interpreter unexpectedly wrote memory for {}", data->name);
      }

      // jMem
      if ((jMem00[i] & ~memMask[i]) != (mem00[i] & ~memMask[i]) ||
         (jMemFF[i] & ~memMask[i]) != (memFF[i] & ~memMask[i])) {
         gLog->warn("JIT unexpectedly wrote memory for {}", data->name);
      }

   }

   // Check all expected result values
   /* DISABLED FOR NOW
   for (auto i = 0; i < memSize; ++i) {
      // Check that the JIT memory changes matched the interpreter
      if ((iMem00[i] & memMask[i]) != (jMem00[i] & memMask[i])) {
         gLog->warn("JIT and Interpreter gave different memory results");
      }
   }
   */

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
