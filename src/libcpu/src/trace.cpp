#include "trace.h"
#include "state.h"
#include "statedbg.h"
#include "cpu.h"
#include "espresso/espresso_disassembler.h"
#include "espresso/espresso_instructionset.h"
#include "espresso/espresso_spr.h"

#include <common/log.h>
#include <common/platform_debug.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>
#include <mutex>

using espresso::Instruction;
using espresso::InstructionID;
using espresso::InstructionInfo;
using espresso::InstructionField;
using espresso::SPR;

//#define TRACE_ENABLED
//#define TRACE_SC_ENABLED
//#define TRACE_VERIFICATION

struct Tracer
{
   size_t index;
   size_t numTraces;
   std::vector<Trace> traces;
   cpu::CoreRegs prevState;
};

static inline void
debugPrint(std::string out)
{
   gLog->debug(out);

   out.push_back('\n');
   platform::debugLog(out);
}

namespace cpu
{

cpu::Tracer *
allocTracer(size_t size)
{
   // TODO: TRACE_ENABLED should actually be handled by kernel
#ifdef TRACE_ENABLED
   auto tracer = new Tracer();
   tracer->index = 0;
   tracer->numTraces = 0;
   tracer->traces.resize(size);
   return tracer;
#else
   return nullptr;
#endif
}

void
freeTracer(cpu::Tracer *tracer)
{
   if (tracer) {
      delete tracer;
   }
}

namespace this_core
{

void setTracer(cpu::Tracer *tracer)
{
   state()->tracer = tracer;
}

} // namespace this_core

} // namespace cpu

std::string
getStateFieldName(TraceFieldType type)
{
   if (type == StateField::Invalid) {
      return "Invalid";
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      return fmt::format("r{:02}", type - StateField::GPR);
   } else if (type >= StateField::FPR0 && type <= StateField::FPR31) {
      return fmt::format("f{:02}", type - StateField::FPR);
   } else if (type >= StateField::GQR0 && type <= StateField::GQR7) {
      return fmt::format("q{:02}", type - StateField::FPR);
   } else if (type == StateField::CR) {
      return "CR";
   } else if (type == StateField::XER) {
      return "XER";
   } else if (type == StateField::LR) {
      return "LR";
   } else if (type == StateField::FPSCR) {
      return "FPSCR";
   } else if (type == StateField::CTR) {
      return "CTR";
   } else {
      decaf_abort(fmt::format("Invalid TraceFieldType {}", static_cast<int>(type)));
   }
}

static void
printFieldValue(fmt::memory_buffer &out, Instruction instr, TraceFieldType type, const TraceFieldValue& value)
{
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      fmt::format_to(out, "    r{:02} = {:08x}\n", type - StateField::GPR, value.u32v0);
   } else if (type == StateField::CR) {
      auto valX = [&](int i) { return (value.u32v0 >> ((i) * 4)) & 0xF; };
      fmt::format_to(out, "    CR = {:04b} {:04b} {:04b} {:04b} {:04b} {:04b} {:04b} {:04b}\n",
         valX(7), valX(6), valX(5), valX(4), valX(3), valX(2), valX(1), valX(0));
   } else if (type == StateField::XER) {
      fmt::format_to(out, "    XER = {:08x}\n", value.u32v0);
   } else if (type == StateField::LR) {
      fmt::format_to(out, "    LR = {:08x}\n", value.u32v0);
   } else if (type == StateField::CTR) {
      fmt::format_to(out, "    CTR = {:08x}\n", value.u32v0);
   } else {
      decaf_abort(fmt::format("Invalid TraceFieldType {}", static_cast<int>(type)));
   }
}

static void
printInstruction(fmt::memory_buffer &out, const Trace& trace, int index)
{
   espresso::Disassembly dis;
   espresso::disassemble(trace.instr, dis, trace.cia);

   std::string addend = "";

   /*
   if (dis.instruction->id == InstructionID::kc) {
      auto scall = gSystem.getSyscallData(trace.instr.li);
      addend = " [" + std::string(scall->name) + "]";
   }
   */

   for (auto &write : trace.writes) {
      printFieldValue(out, trace.instr, write.type, write.value);
   }

   fmt::format_to(out, "  [{}] {:08x} {}{}\n", index, trace.cia, dis.text.c_str(), addend.c_str());

   for (auto &read : trace.reads) {
      printFieldValue(out, trace.instr, read.type, read.value);
   }
}

const Trace&
getTrace(Tracer *tracer, int index)
{
   auto tracerSize = tracer->numTraces;
   decaf_check(index >= 0);
   decaf_check(index < tracerSize);
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
traceInit(cpu::Core *state, size_t size)
{
#ifdef TRACE_ENABLED
   state->tracer = new Tracer();
   state->tracer->index = 0;
   state->tracer->numTraces = 0;
   state->tracer->traces.resize(size);
#else
   state->tracer = nullptr;
#endif
}

static uint32_t
getFieldStateField(Instruction instr, InstructionField field)
{
   switch (field) {
   case InstructionField::rA:
      return StateField::GPR + instr.rA;
   case InstructionField::rB:
      return StateField::GPR + instr.rB;
   case InstructionField::rS:
      return StateField::GPR + instr.rS;
   case InstructionField::rD:
      return StateField::GPR + instr.rD;
   case InstructionField::frA:
      return StateField::FPR + instr.frA;
   case InstructionField::frB:
      return StateField::FPR + instr.frB;
   case InstructionField::frC:
      return StateField::FPR + instr.frC;
   case InstructionField::frD:
      return StateField::FPR + instr.frD;
   case InstructionField::frS:
      return StateField::FPR + instr.frS;
   case InstructionField::spr:
      switch (decodeSPR(instr)) {
      case SPR::CTR:
         return StateField::CTR;
      case SPR::LR:
         return StateField::LR;
      case SPR::XER:
         return StateField::XER;
      case SPR::GQR0:
         return StateField::GQR + 0;
      case SPR::GQR1:
         return StateField::GQR + 1;
      case SPR::GQR2:
         return StateField::GQR + 2;
      case SPR::GQR3:
         return StateField::GQR + 3;
      case SPR::GQR4:
         return StateField::GQR + 4;
      case SPR::GQR5:
         return StateField::GQR + 5;
      case SPR::GQR6:
         return StateField::GQR + 6;
      case SPR::GQR7:
         return StateField::GQR + 7;
      default:
         break;
      }
      break;
   case InstructionField::bo:
      if ((instr.bo & 4) == 0) {
         return StateField::CTR;
      }
      break;
   case InstructionField::bi:
      return StateField::CR;
   case InstructionField::crbA:
   case InstructionField::crbB:
   case InstructionField::crbD:
   case InstructionField::crfD:
   case InstructionField::crfS:
   case InstructionField::crm:
      return StateField::CR;
   case InstructionField::oe:
      if (instr.oe) {
         return StateField::XER;
      }
      break;
   case InstructionField::rc:
      if (instr.rc) {
         return StateField::CR;
      }
      break;
   case InstructionField::lk:
      return StateField::LR;
   case InstructionField::XERO:
      return StateField::XER;
   case InstructionField::CTR:
      return StateField::CTR;
   case InstructionField::LR:
      return StateField::LR;
   case InstructionField::FPSCR:
      return StateField::FPSCR;
   case InstructionField::RSRV:
      // TODO: Handle this?
   default:
      break;
   }
   return StateField::Invalid;
}

void
saveStateField(const cpu::CoreRegs *state, TraceFieldType type, TraceFieldValue &field)
{
   field.u64v0 = 0;
   field.u64v1 = 0;

   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      field.u32v0 = state->gpr[type - StateField::GPR];
   } else if (type >= StateField::FPR0 && type <= StateField::FPR31) {
      field.u64v0 = state->fpr[type - StateField::FPR].idw;
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
   } else {
      decaf_abort(fmt::format("Invalid TraceFieldType {}", static_cast<int>(type)));
   }
}

void
restoreStateField(cpu::CoreRegs *state, TraceFieldType type, const TraceFieldValue &field)
{
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      state->gpr[type - StateField::GPR] = field.u32v0;
   } else if (type >= StateField::FPR0 && type <= StateField::FPR31) {
      state->fpr[type - StateField::FPR].idw = field.u64v0;
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
   } else {
      decaf_abort(fmt::format("Invalid TraceFieldType {}", static_cast<int>(type)));
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
traceInstructionStart(Instruction instr, InstructionInfo *data, cpu::Core *state)
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

   // TODO: This is a bit of a hack... We should probably not do this...
   tracer->prevState = *state;
   return &trace;
}

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionInfo *data, cpu::Core *state)
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
tracePrint(cpu::Core *state, int start, int count)
{
   auto tracer = state->tracer;
   if (!tracer) {
      debugPrint("Tracing is disabled");
      return;
   }

   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));

   if (count == 0) {
      count = tracerSize - start;
   }

   auto end = start + count;
   decaf_check(start >= 0);
   decaf_check(end <= tracerSize);

   fmt::memory_buffer out;
   fmt::format_to(out, "Trace - Print {} to {}\n", start, end);

   for (auto i = start; i < end; ++i) {
      auto &trace = getTrace(tracer, i);
      printInstruction(out, trace, i);
   }

   debugPrint(to_string(out));
}

int
traceReg(cpu::Core *state, int start, int regIdx)
{
   auto tracer = state->tracer;
   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));
   bool found = false;

   fmt::memory_buffer out;
   fmt::format_to(out, "Trace - Search {} to {} for write r{}\n", start, tracerSize, regIdx);

   decaf_check(start >= 0);
   decaf_check(start < tracerSize);

   for (auto i = start; i < tracerSize; ++i) {
      auto &trace = getTrace(tracer, i);

      bool wasMatchedWrite = false;
      for (auto &j : trace.writes) {
         if (j.type == StateField::GPR + regIdx) {
            wasMatchedWrite = true;
            break;
         }
      }

      if (wasMatchedWrite) {
         printInstruction(out, trace, i);
         found = true;
         break;
      }
   }

   if (!found) {
      fmt::format_to(out, "  Nothing Found");
   }

   debugPrint(to_string(out));
   return -1;
}

static cpu::Core *gRegTraceState = nullptr;
static int gRegTraceIndex = 0;
static int gRegTraceNextReg = -1;

void
traceRegStart(cpu::Core *state, int start, int regIdx)
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
         debugPrint(fmt::format("Suggested next register is r{}", gRegTraceNextReg));
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

std::list<std::string> gSyscallTrace;
std::mutex gSyscallTraceLock;

void
tracePrintSyscall(int count)
{
   std::unique_lock<std::mutex> lock(gSyscallTraceLock);

   if (count <= 0 || count >= gSyscallTrace.size()) {
      count = (int)gSyscallTrace.size();
   }

   fmt::memory_buffer out;
   fmt::format_to(out, "Trace - Last {} syscalls\n", count);

   int j = 0;
   for (auto i = gSyscallTrace.begin(); i != gSyscallTrace.end() && j < count; ++i, ++j) {
      fmt::format_to(out, "  {}\n", *i);
   }

   debugPrint(to_string(out));
}

void
traceLogSyscall(const std::string &info)
{
#ifdef TRACE_SC_ENABLED
   std::unique_lock<std::mutex> lock(gSyscallTraceLock);

   gSyscallTrace.push_front(info);
   while (gSyscallTrace.size() > 2048) {
      gSyscallTrace.pop_back();
   }
#endif
}
