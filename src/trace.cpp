#include "cpu/disassembler.h"
#include "cpu/instruction.h"
#include "cpu/instructiondata.h"
#include "log.h"
#include "memory.h"
#include "cpu/state.h"
#include "trace.h"
#include "system.h"
#include "kernelfunction.h"
#include "statedbg.h"

//#define TRACE_VERIFICATION

template<typename... Args>
static void
debugPrint(fmt::StringRef msg, Args... args) {
   auto out = fmt::format(msg, args...);
   out += "\n";
   OutputDebugStringA(out.c_str());
}

struct Tracer
{
   size_t index;
   size_t numTraces;
   std::vector<Trace> traces;
   ThreadState prevState;
};

static void
printFieldValue(Instruction instr, TraceFieldType type, const TraceFieldValue& value)
{
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      debugPrint("    r{:02} = {:08x}", type - StateField::GPR, value.u32v0);
   } else if (type == StateField::CR) {
      auto valX = [&](int i) { return (value.u32v0 >> ((i) * 4)) & 0xF; };
      debugPrint("    CR = {:04b} {:04b} {:04b} {:04b} {:04b} {:04b} {:04b} {:04b}",
         valX(0), valX(1), valX(2), valX(3), valX(4), valX(5), valX(6), valX(7));
   } else if (type == StateField::XER) {
      debugPrint("    XER = {:08x}", value.u32v0);
   } else if (type == StateField::LR) {
      debugPrint("    LR = {:08x}", value.u32v0);
   } else if (type == StateField::CTR) {
      debugPrint("    CTR = {:08x}", value.u32v0);
   } else {
      assert(0);
   }
}

static void
printInstruction(const Trace& trace, int index)
{
   Disassembly dis;
   gDisassembler.disassemble(trace.instr, dis, trace.cia);

   std::string addend = "";

   if (dis.instruction->id == InstructionID::kc) {
      auto scall = gSystem.getSyscall(trace.instr.li);
      addend = " [" + std::string(scall->name) + "]";
   }

   for (auto &write : trace.writes) {
      printFieldValue(trace.instr, write.type, write.value);
   }
   
   debugPrint("  [{}] {:08x} {}{}", index, trace.cia, dis.text.c_str(), addend.c_str());

   for (auto &read : trace.reads) {
      printFieldValue(trace.instr, read.type, read.value);
   }
}

const Trace&
getTrace(Tracer *tracer, int index)
{
   auto tracerSize = tracer->numTraces;
   assert(index >= 0);
   assert(index < tracerSize);
   auto realIndex = (int)tracer->index - 1 - index;
   while (realIndex < 0) {
      realIndex += (int)tracerSize;
   }
   while (realIndex >= tracerSize) {
      realIndex -= (int)tracerSize;
   }
   return tracer->traces[realIndex];
}

size_t
getTracerNumTraces(Tracer *tracer)
{
   return tracer->numTraces;
}

void
traceInit(ThreadState *state, size_t size)
{
   state->tracer = new Tracer();
   state->tracer->index = 0;
   state->tracer->numTraces = 0;
   state->tracer->traces.resize(size);
}

static SprEncoding
decodeSPR(Instruction instr)
{
   return static_cast<SprEncoding>(((instr.spr << 5) & 0x3E0) | ((instr.spr >> 5) & 0x1F));
}

static uint32_t
getFieldStateField(Instruction instr, Field field)
{
   switch (field) {
   case Field::rA:
      return StateField::GPR + instr.rA;
   case Field::rB:
      return StateField::GPR + instr.rB;
   case Field::rS:
      return StateField::GPR + instr.rS;
   case Field::rD:
      return StateField::GPR + instr.rD;
   case Field::frA:
      return StateField::FPR + instr.frA;
   case Field::frB:
      return StateField::FPR + instr.frB;
   case Field::frC:
      return StateField::FPR + instr.frC;
   case Field::frD:
      return StateField::FPR + instr.frD;
   case Field::frS:
      return StateField::FPR + instr.frS;
   case Field::spr:
      switch (decodeSPR(instr)) {
      case SprEncoding::CTR:
         return StateField::CTR;
      case SprEncoding::LR:
         return StateField::LR;
      case SprEncoding::XER:
         return StateField::XER;
      case SprEncoding::GQR0:
         return StateField::GQR + 0;
      case SprEncoding::GQR1:
         return StateField::GQR + 1;
      case SprEncoding::GQR2:
         return StateField::GQR + 2;
      case SprEncoding::GQR3:
         return StateField::GQR + 3;
      case SprEncoding::GQR4:
         return StateField::GQR + 4;
      case SprEncoding::GQR5:
         return StateField::GQR + 5;
      case SprEncoding::GQR6:
         return StateField::GQR + 6;
      case SprEncoding::GQR7:
         return StateField::GQR + 7;
      }
      break;
   case Field::bo:
      if ((instr.bo & 4) == 0) {
         return StateField::CTR;
      }
      break;
   case Field::bi:
      return StateField::CR;
   case Field::crbA:
   case Field::crbB:
   case Field::crbD:
   case Field::crfD:
   case Field::crfS:
   case Field::crm:
      return StateField::CR;
   case Field::oe:
      if (instr.oe) {
         return StateField::XER;
      }
      break;
   case Field::rc:
      if (instr.rc) {
         return StateField::CR;
      }
      break;
   case Field::lk:
      return StateField::LR;
   case Field::XER:
      return StateField::XER;
   case Field::CR:
      return StateField::CR;
   case Field::CTR:
      return StateField::CTR;
   case Field::LR:
      return StateField::LR;
   case Field::FPSCR:
      return StateField::FPSCR;
   case Field::RSRV:
      return StateField::ReserveAddress;
   }
   return StateField::Invalid;
}

void
saveStateField(const ThreadState *state, TraceFieldType type, TraceFieldValue &field)
{
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      field.u32v0 = state->gpr[type - StateField::GPR];
   } else if (type >= StateField::FPR0 && type <= StateField::FPR31) {
      field.u64v0 = state->fpr[type - StateField::FPR].value0;
      field.u64v1 = state->fpr[type - StateField::FPR].value1;
   } else if (type >= StateField::GQR0 && type <= StateField::GQR7) {
      field.u32v0 = state->gqr[type - StateField::GQR].value;
   } else if (type == StateField::CR) {
      field.u32v0 = state->cr.value;
   } else if (type == StateField::XER) {
      field.u32v0 = state->xer.value;
   } else if (type == StateField::LR) {
      field.u32v0 = state->lr;
   } else if (type == StateField::CTR) {
      field.u32v0 = state->ctr;
   } else if (type == StateField::FPSCR) {
      field.u32v0 = state->fpscr.value;
   } else if (type == StateField::ReserveAddress) {
      field.u32v0 = state->reserve ? 1 : 0;
      field.u32v1 = state->reserveAddress;
      field.u32v2 = state->reserveData;
   } else {
      assert(0);
   }
}

void
restoreStateField(ThreadState *state, TraceFieldType type, const TraceFieldValue &field)
{
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      state->gpr[type - StateField::GPR] = field.u32v0;
   } else if (type >= StateField::FPR0 && type <= StateField::FPR31) {
      state->fpr[type - StateField::FPR].value0 = field.u64v0;
      state->fpr[type - StateField::FPR].value1 = field.u64v1;
   } else if (type >= StateField::GQR0 && type <= StateField::GQR7) {
      state->gqr[type - StateField::GQR].value = field.u32v0;
   } else if (type == StateField::CR) {
      state->cr.value = field.u32v0;
   } else if (type == StateField::XER) {
      state->xer.value = field.u32v0;
   } else if (type == StateField::LR) {
      state->lr = field.u32v0;
   } else if (type == StateField::CTR) {
      state->ctr = field.u32v0;
   } else if (type == StateField::FPSCR) {
      state->fpscr.value = field.u32v0;
   } else if (type == StateField::ReserveAddress) {
      state->reserve = (field.u32v0 != 0);
      state->reserveAddress = field.u32v1;
      state->reserveData = field.u32v2;
   } else {
      assert(0);
   }
}

template<typename T>
static void
pushUniqueField(std::vector<T> &fields, uint32_t fieldId)
{
   if (fieldId == StateField::Invalid) {
      return;
   }

   for (auto &i : fields) {
      if (i.type == fieldId) {
         return;
      }
   }

   fields.push_back({ fieldId });
}

Trace *
traceInstructionStart(Instruction instr, InstructionData *data, ThreadState *state)
{
   if (!state->tracer) {
      return nullptr;
   }

   auto tracer = state->tracer;
   auto &trace = tracer->traces[tracer->index];
   auto tracerSize = tracer->traces.size();

   if (tracer->numTraces < tracerSize) {
      tracer->numTraces++;
   }

   tracer->index = (tracer->index + 1) % tracerSize;

   // Setup Trace
   trace.instr = instr;
   trace.cia = state->cia;
   trace.reads.clear();
   trace.writes.clear();

   // Automatically determine register changes
   for (auto i = 0u; i < data->read.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->read[i]);
      pushUniqueField(trace.reads, stateField);
   }

   for (auto i = 0u; i < data->write.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->write[i]);
      pushUniqueField(trace.writes, stateField);
   }

   for (auto i = 0u; i < data->flags.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->flags[i]);
      pushUniqueField(trace.writes, stateField);
   }

   // Some special cases.
   if (data->id == InstructionID::lmw) {
      for (uint32_t i = StateField::GPR + instr.rD; i <= StateField::GPR31; ++i) {
         pushUniqueField(trace.writes, i);
      }
   } else if (data->id == InstructionID::stmw) {
      for (uint32_t i = StateField::GPR + instr.rS; i <= StateField::GPR31; ++i) {
         pushUniqueField(trace.reads, i);
      }
   } else if (data->id == InstructionID::stswi) {
      // TODO: Implement Me
   } else if (data->id == InstructionID::stswx) {
      // TODO: Implement Me
   }

#ifdef TRACE_VERIFICATION
   if (data->id == InstructionID::stswi) {
      //assert(0);
   } else if (data->id == InstructionID::stswx) {
      //assert(0);
   }
#endif

   // Save all state
   for (auto &i : trace.reads) {
      saveStateField(state, i.type, i.value);
   }

   for (auto &i : trace.writes) {
      saveStateField(state, i.type, i.prevalue);
   }

   tracer->prevState = *state;
   return &trace;
}

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionData *data, ThreadState *state)
{
   if (!trace) {
      return;
   }

   auto tracer = state->tracer;
   
   // Special hack for KC for now
   if (data->id == InstructionID::kc) {
      trace->writes.clear();

      for (int i = 0; i < StateField::Max; ++i) {
         TraceFieldValue curVal, prevVal;
         saveStateField(&tracer->prevState, i, prevVal);
         saveStateField(state, i, curVal);
         if (curVal.value != prevVal.value) {
            pushUniqueField(trace->writes, i);
         }
      }

      for (auto &i : trace->writes) {
         saveStateField(&tracer->prevState, i.type, i.prevalue);
      }
   }

   for (auto &i : trace->writes) {
      saveStateField(state, i.type, i.value);
   }

#ifdef TRACE_VERIFICATION
   if (tracer->numTraces > 0) {
      auto errors = std::vector<std::string> {};
      auto checkState = *state;
      checkState.nia = tracer->prevState.nia;

      for (auto &i : trace->writes) {
         restoreStateField(&checkState, i.type, i.prevalue);
      }

      if (!dbgStateCmp(&checkState, &tracer->prevState, errors)) {
         gLog->error("Trace Compliance errors w/ {}", data->name);

         for (auto &err : errors) {
            gLog->error(err);
         }

         DebugBreak();
      }
   }
#endif
}

void
tracePrint(ThreadState *state, int start, int count)
{
   auto tracer = state->tracer;
   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));

   if (count == 0) {
      count = tracerSize - start;
   }

   auto end = start + count;
   assert(start >= 0);
   assert(end <= tracerSize);

   debugPrint("Trace - Print {} to {}", start, end);

   for (auto i = start; i < end; ++i) {
      auto &trace = getTrace(tracer, i);
      printInstruction(trace, i);
   }
}

int
traceReg(ThreadState *state, int start, int regIdx)
{
   auto tracer = state->tracer;
   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));

   debugPrint("Trace - Search {} to {} for write r{}", start, tracerSize, regIdx);

   assert(start >= 0);
   assert(start < tracerSize);

   for (auto i = start; i < tracerSize; ++i) {
      auto &trace = getTrace(tracer, i);

      bool wasMatchedWrite = false;
      for (auto &i : trace.writes) {
         if (i.type == StateField::GPR + regIdx) {
            wasMatchedWrite = true;
            break;
         }
      }

      if (wasMatchedWrite) {
         printInstruction(trace, i);
         return i;
      }
   }

   debugPrint("  Nothing Found");
   return -1;
}

static ThreadState *gRegTraceState = nullptr;
static int gRegTraceIndex = 0;
static int gRegTraceNextReg = -1;

void
traceRegStart(ThreadState *state, int start, int regIdx)
{
   gRegTraceState = state;
   gRegTraceIndex = start;
   gRegTraceNextReg = -1;
   traceRegNext(regIdx);
}

void
traceRegNext(int regIdx)
{
   if (!gRegTraceState || gRegTraceIndex < 0) {
      debugPrint("Need to use traceRegStart first.");
      return;
   }

   auto foundIndex = traceReg(gRegTraceState, gRegTraceIndex, regIdx);
   if (foundIndex == -1) {
      gRegTraceIndex = -1;
      return;
   }

   auto tracer = gRegTraceState->tracer;
   auto &trace = getTrace(tracer, foundIndex);

   if (trace.reads.size() == 1) {
      if (trace.reads.front().type >= StateField::GPR0 && trace.reads.front().type <= StateField::GPR31) {
         gRegTraceNextReg = trace.reads.front().type - StateField::GPR;
         debugPrint("Suggested next register is r{}", gRegTraceNextReg);
      } else {
         // Not a GPR read
         gRegTraceNextReg = -1;
      }
   } else {
      // More than one read field
      gRegTraceNextReg = -1;
   }

   gRegTraceIndex = foundIndex + 1;
}

void
traceRegAround()
{
   if (!gRegTraceState || gRegTraceIndex < 0) {
      debugPrint("Need to use traceRegStart first.");
      return;
   }

   int start = gRegTraceIndex - 5;
   int end = gRegTraceIndex + 4;

   auto tracer = gRegTraceState->tracer;
   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));

   if (start < 0) {
      start = 0;
   }
   if (end > tracerSize) {
      end = tracerSize;
   }

   tracePrint(gRegTraceState, start, end - start);
}

void
traceRegContinue()
{
   if (gRegTraceNextReg == -1) {
      debugPrint("No stored suggested next register.");
      return;
   }

   traceRegNext(gRegTraceNextReg);
}
