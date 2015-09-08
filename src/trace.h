#pragma once
#include "instruction.h"

struct InstructionData;
struct ThreadState;
struct Tracer;

// TODO: Probably should rename this to something reasonable
namespace StateField {
   enum Field : uint32_t {
      Invalid = 0,
      GPR = 1,
      // 1 - 32 == GPR[0] to GPR[31]
      FPR = 33,
      // 33 - 64 == FPR[0] to FPR[31]
      LR = 65,
      CTR = 66,
      CR = 67,
      XER = 68,
   };
}

struct Trace
{
   Instruction instr;
   InstructionData *data;
   uint32_t cia;
   uint32_t read[4];
   uint32_t write[4];
};

const Trace& getTrace(Tracer *tracer, int index);

size_t getTracerNumTraces(Tracer *tracer);

void
traceInit(ThreadState *state, size_t size);

Trace *
traceInstructionStart(Instruction instr, InstructionData *data, ThreadState *state);

void
traceInstructionEnd(Trace *trace, Instruction instr, InstructionData *data, ThreadState *state);

void
tracePrint(ThreadState *state, int start, int count);

int
traceReg(ThreadState *state, int start, int regIdx);

void
traceRegStart(ThreadState *state, int start, int regIdx);

void
traceRegNext(int regIdx);

void
traceRegContinue();
