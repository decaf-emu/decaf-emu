#include <vector>
#include "cpu.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "instructiondata.h"

namespace cpu
{

JitMode gJitMode = JitMode::Disabled;

void setJitMode(JitMode mode)
{
   gJitMode = mode;
}

void initialise()
{
   gInstructionTable.initialise();
   cpu::interpreter::initialise();
   cpu::jit::initialise();
}

void executeSub(ThreadState *state)
{
   if (gJitMode == JitMode::Enabled) {
      jit::executeSub(state);
   } else {
      interpreter::executeSub(state);
   }
}

}