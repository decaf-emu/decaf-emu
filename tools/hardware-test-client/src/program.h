#ifndef PROGRAM_H
#define PROGRAM_H
#include "../../../libwiiu/src/coreinit.h"
#include "../../../libwiiu/src/vpad.h"
#include "../../../libwiiu/src/types.h"
#include "../../../libwiiu/src/draw.h"
#include "../../../libwiiu/src/socket.h"

struct TestState
{
   uint32_t xer;     // 0x00
   uint32_t cr;      // 0x04
   uint32_t ctr;     // 0x0c
   uint32_t _;       // 0x08
   uint32_t r3;      // 0x10
   uint32_t r4;      // 0x14
   uint32_t r5;      // 0x18
   uint32_t r6;      // 0x1C
   uint64_t fpscr;   // 0x20
   double f1;        // 0x28
   double f2;        // 0x30
   double f3;        // 0x38
   double f4;        // 0x40
}; // 0x48

extern void executeCodeTest(struct TestState *state, void *func);

void _entryPoint();

#endif /* PROGRAM_H */