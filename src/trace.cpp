#include "disassembler.h"
#include "instruction.h"
#include "instructiondata.h"
#include "log.h"
#include "memory.h"
#include "ppc.h"
#include "trace.h"
#include "system.h"
#include "kernelfunction.h"

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

   for (auto j = 0; j < NumTraceWriteFields; ++j) {
      printFieldValue(trace.instr, trace.writeField[j], trace.writeValue[j]);
   }
   
   debugPrint("  [{}] {:08x} {}{}", index, trace.cia, dis.text.c_str(), addend.c_str());

   for (auto j = 0; j < NumTraceReadFields; ++j) {
      printFieldValue(trace.instr, trace.readField[j], trace.readValue[j]);
   }
}

const Trace& getTrace(Tracer *tracer, int index)
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

size_t getTracerNumTraces(Tracer *tracer) {
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

static uint32_t getFieldStateField(Instruction instr, Field field) {
   switch (field) {
   case Field::rA:
      return StateField::GPR + instr.rA;
   case Field::rB:
      return StateField::GPR + instr.rB;
   case Field::rS:
      return StateField::GPR + instr.rS;
   case Field::rD:
      return StateField::GPR + instr.rD;
   case Field::spr:
      switch (decodeSPR(instr)) {
      case SprEncoding::CTR:
         return StateField::CTR;
      case SprEncoding::LR:
         return StateField::LR;
      case SprEncoding::XER:
         return StateField::XER;
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
   case Field::XER:
      return StateField::XER;
   case Field::CTR:
      return StateField::CTR;
   case Field::LR:
      return StateField::LR;
   case Field::lk:
      return StateField::LR;
   }
   return StateField::Invalid;
}

template<int Size>
static void pushUniqueField(TraceFieldType (&fields)[Size], uint32_t fieldId) {
   if (fieldId == StateField::Invalid) {
      return;
   }

   for (int i = 0; i < Size; ++i) {
      if (fields[i] == fieldId) {
         // Already in the list
         return;
      } else if (fields[i] == StateField::Invalid) {
         fields[i] = fieldId;
         return;
      }
   }
   assert(0);
}

static void populateStateField(ThreadState *state, TraceFieldType type, TraceFieldValue &field) {
   if (type == StateField::Invalid) {
      return;
   }

   if (type >= StateField::GPR0 && type <= StateField::GPR31) {
      field.u32v0 = state->gpr[type - StateField::GPR];
   } else if (type == StateField::CR) {
      field.u32v0 = state->cr.value;
   } else if (type == StateField::XER) {
      field.u32v0 = state->xer.value;
   } else if (type == StateField::LR) {
      field.u32v0 = state->lr;
   } else if (type == StateField::CTR) {
      field.u32v0 = state->ctr;
   } else {
      assert(0);
   }
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
   
   memset(&trace, 0xDC, sizeof(Trace));
   trace.instr = instr;
   trace.cia = state->cia;

   for (int i = 0; i < NumTraceReadFields; ++i) {
      trace.readField[i] = StateField::Invalid;
   }
   for (int i = 0; i < NumTraceWriteFields; ++i) {
      trace.writeField[i] = StateField::Invalid;
   }

   for (auto i = 0u; i < data->read.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->read[i]);
      pushUniqueField(trace.readField, stateField);
   }

   for (auto i = 0u; i < data->write.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->write[i]);
      pushUniqueField(trace.writeField, stateField);
   }
   for (auto i = 0u; i < data->flags.size(); ++i) {
      auto stateField = getFieldStateField(instr, data->flags[i]);
      pushUniqueField(trace.writeField, stateField);
   }

   for (int i = 0; i < NumTraceReadFields; ++i) {
      populateStateField(state, trace.readField[i], trace.readValue[i]);
   }
   for (int i = 0; i < NumTraceWriteFields; ++i) {
      populateStateField(state, trace.writeField[i], trace.prewriteValue[i]);
   }

   return &trace;
}

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionData *data, ThreadState *state)
{
   if (!trace) {
      return;
   }

   for (int i = 0; i < NumTraceWriteFields; ++i) {
      populateStateField(state, trace->writeField[i], trace->writeValue[i]);
   }
}

void
tracePrint(ThreadState *state, int start, int count)
{
   auto tracer = state->tracer;
   auto tracerSize = static_cast<int>(getTracerNumTraces(tracer));

   if (count == 0) {
      count = tracerSize - start;
   }

   int end = start + count;
   
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
      for (auto i = 0; i < NumTraceWriteFields; ++i) {
         if (trace.writeField[i] == StateField::GPR + regIdx) {
            wasMatchedWrite = true;
            break;
         }
      }

      // TODO: Handle kc's return values...

      if (wasMatchedWrite) {
         printInstruction(trace, i);
         return i;
      }
   }

   debugPrint("  Nothing Found");
   return -1;
}

ThreadState *gRegTraceState = nullptr;
int gRegTraceIndex = 0;
int gRegTraceNextReg = -1;

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

   if (trace.readField[0] != StateField::Invalid && trace.readField[1] == StateField::Invalid) {
      if (trace.readField[0] >= StateField::GPR0 && trace.readField[0] <= StateField::GPR31) {
         gRegTraceNextReg = trace.readField[0] - StateField::GPR;
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