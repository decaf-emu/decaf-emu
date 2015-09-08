#pragma once
#include "instruction.h"

struct InstructionData;
struct ThreadState;
struct Tracer;

// TODO: Probably should rename this to something reasonable
namespace StateField {
   enum Field : uint8_t {
      Invalid,
      GPR,
      GPR0 = GPR,
      GPR31 = GPR + 31,
      FPR,
      FPR0 = FPR,
      FPR31 = FPR + 31,
      CR,
      XER,
      LR,
      CTR,
      FPSCR,
      Reserved,
      ReservedAddress,
      Memory,
      Max,
   };
}

typedef uint32_t TraceFieldType;

struct TraceFieldValue {
   union {
      struct {
         uint32_t u32v0;
         uint32_t u32v1;
      };
      uint64_t u64v;
      struct {
         float f32v0;
         float f32v1;
      };
      double f64v;
      struct {
         uint32_t mem_size;
         uint32_t mem_offset;
      };
      uint64_t value;
   };
};

static const int NumTraceReadFields = 4;
static const int NumTraceWriteFields = 4;

struct Trace
{
   Instruction instr;
   uint32_t cia;
   TraceFieldType readField[NumTraceReadFields];
   TraceFieldValue readValue[NumTraceReadFields];
   TraceFieldType writeField[NumTraceWriteFields];
   TraceFieldValue writeValue[NumTraceWriteFields];
   TraceFieldValue prewriteValue[NumTraceWriteFields];
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
