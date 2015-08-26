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

struct Trace
{
   Instruction instr;
   InstructionData *data;
   uint32_t cia;
   uint32_t read[4];
   uint32_t write[4];
};

struct Tracer
{
   size_t index;
   std::vector<Trace> trace;
};

static uint32_t
getFieldValue(ThreadState *state, Instruction instr, Field field)
{
   switch (field) {
   case Field::rA:
      return state->gpr[instr.rA];
   case Field::rB:
      return state->gpr[instr.rB];
   case Field::rS:
      return state->gpr[instr.rS];
   case Field::rD:
      return state->gpr[instr.rD];
   }

   return 0xDCDCDCDC;
}

static uint32_t
getFieldReg(Instruction instr, Field field)
{
   switch (field) {
   case Field::rA:
      return instr.rA;
   case Field::rB:
      return instr.rB;
   case Field::rS:
      return instr.rS;
   case Field::rD:
      return instr.rD;
   }
   return -1;
}

static void
printFieldValue(Instruction instr, Field field, uint32_t value)
{
   uint32_t regIdx = getFieldReg(instr, field);
   if (regIdx != -1) {
      debugPrint("    r{:02} = {:08x}", regIdx, value);
   }
}

static void
printInstruction(const Trace& trace, int index)
{
   Disassembly dis;
   gDisassembler.disassemble(trace.instr, dis);

   std::string addend = "";

   if (dis.instruction->id == InstructionID::kc) {
      auto scall = gSystem.getSyscall(trace.instr.li);
      addend = " [" + std::string(scall->name) + "]";
   }


   for (auto j = 0u; j < trace.data->write.size(); ++j) {
      printFieldValue(trace.instr, trace.data->write[j], trace.write[j]);
   }

   debugPrint("  [{}] {:08x} {}{}", index, trace.cia, dis.text.c_str(), addend.c_str());

   for (auto j = 0u; j < trace.data->read.size(); ++j) {
      printFieldValue(trace.instr, trace.data->read[j], trace.read[j]);
   }

   
}

void
traceInit(ThreadState *state, size_t size)
{
   state->tracer = new Tracer();
   state->tracer->index = 0;
   state->tracer->trace.resize(size);
}

Trace *
traceInstructionStart(Instruction instr, InstructionData *data, ThreadState *state)
{
   if (!state->tracer) {
      return nullptr;
   }

   auto tracer = state->tracer;
   auto &trace = tracer->trace[tracer->index];
   tracer->index = (tracer->index + 1) % tracer->trace.size();
   
   memset(&trace, 0xDC, sizeof(Trace));
   trace.data = data;
   trace.instr = instr;
   trace.cia = state->cia;

   for (auto i = 0u; i < data->read.size(); ++i) {
      auto field = data->read[i];
      trace.read[i] = getFieldValue(state, instr, field);
   }

   return &trace;
}

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionData *data, ThreadState *state)
{
   if (!trace) {
      return;
   }

   for (auto i = 0u; i < data->write.size(); ++i) {
      auto field = data->write[i];
      trace->write[i] = getFieldValue(state, instr, field);
   }
}

static const Trace& getTrace(Tracer *tracer, int index)
{
   auto tracerSize = tracer->trace.size();
   assert(index >= 0);
   assert(index < tracerSize);
   auto realIndex = tracer->index - 1 - index;
   while (realIndex < 0) {
      realIndex += tracerSize;
   }
   while (realIndex >= tracerSize) {
      realIndex -= tracerSize;
   }
   return tracer->trace[realIndex];
}

void
tracePrint(ThreadState *state, int start, int count)
{
   auto tracer = state->tracer;
   auto tracerSize = static_cast<int>(tracer->trace.size());

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
   auto tracerSize = static_cast<int>(tracer->trace.size());

   debugPrint("Trace - Search {} to {} for write r{}", start, tracerSize, regIdx);

   assert(start >= 0);
   assert(start < tracerSize);

   for (auto i = start; i < tracerSize; ++i) {
      auto &trace = getTrace(tracer, i);

      bool wasMatchedWrite = false;
      for (auto j = 0u; j < trace.data->write.size(); ++j) {
         int writeReg = getFieldReg(trace.instr, trace.data->write[j]);

         if (writeReg == regIdx) {
            wasMatchedWrite = true;
         }
      }

      if (trace.data->id == InstructionID::kc) {
         debugPrint("  Possible Match via KC");
         printInstruction(trace, i);
      }

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

   std::vector<int> readRegs;
   for (auto j = 0u; j < trace.data->read.size(); ++j) {
      int readReg = getFieldReg(trace.instr, trace.data->read[j]);
      if (readReg == -1) {
         // Ignore non-register stuff...
         continue;
      }

      bool alreadySaved = false;
      for (auto k : readRegs) {
         if (k == readReg) {
            alreadySaved = true;
            break;
         }
      }
      if (!alreadySaved) {
         readRegs.push_back(readReg);
      }
   }

   if (readRegs.size() == 1) {
      gRegTraceNextReg = readRegs[0];
      debugPrint("Suggested next register is r{}", gRegTraceNextReg);
   } else {
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
   auto tracerSize = static_cast<int>(tracer->trace.size());

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