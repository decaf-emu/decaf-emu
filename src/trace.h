#pragma once
#include "instruction.h"

struct Trace;
struct InstructionData;
struct ThreadState;

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