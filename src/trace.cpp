#include "disassembler.h"
#include "instruction.h"
#include "instructiondata.h"
#include "log.h"
#include "memory.h"
#include "ppc.h"
#include "trace.h"

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

static void
printFieldValue(Instruction instr, Field field, uint32_t value)
{
   switch (field) {
   case Field::rA:
      gLog->emerg("    r{:02} = {:08x}", instr.rA, value);
      break;
   case Field::rB:
      gLog->emerg("    r{:02} = {:08x}", instr.rB, value);
      break;
   case Field::rS:
      gLog->emerg("    r{:02} = {:08x}", instr.rS, value);
      break;
   case Field::rD:
      gLog->emerg("    r{:02} = {:08x}", instr.rD, value);
      break;
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

void
tracePrint(ThreadState *state, size_t count)
{
   auto tracer = state->tracer;
   auto index = tracer->index;

   // Go back one 
   if (index == 0) {
      index = tracer->trace.size() - 1;
   } else {
      --index;
   }

   for (auto i = 0u; i < count; ++i) {
      Disassembly dis;
      auto &trace = tracer->trace[index];
      gDisassembler.disassemble(trace.instr, dis);

      for (auto j = 0u; j < trace.data->write.size(); ++j) {
         printFieldValue(trace.instr, trace.data->write[j], trace.write[j]);
      }

      gLog->emerg("  {:08x} {}", trace.cia, dis.text.c_str());

      for (auto j = 0u; j < trace.data->read.size(); ++j) {
         printFieldValue(trace.instr, trace.data->read[j], trace.read[j]);
      }

      if (index == 0) {
         index = tracer->trace.size() - 1;
      } else {
         index--;
      }
   }
}
