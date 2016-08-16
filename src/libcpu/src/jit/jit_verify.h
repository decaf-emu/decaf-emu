#include "jit_internal.h"
#include "state.h"

namespace cpu
{

namespace jit
{

struct
VerifyBuffer
{
   CoreRegs coreRegsCopy;       // Copy of core state before JIT execution

   uint32_t memoryAddress;      // Address accessed by instruction (if any)
   uint32_t memorySize;         // Number of bytes accessed by instruction
   uint8_t preJitBuffer[128];   // Copy of memory before JIT execution
   uint8_t postJitBuffer[128];  // Copy of memory as written by JIT code
};

void
insertVerifyCall(PPCEmuAssembler &a,
                 uint32_t instr,
                 void *verifyWrapper);

void
verifyPre(Core *core,
          VerifyBuffer *verifyBuf,
          uint32_t cia,
          uint32_t instr);

void
verifyPost(Core *core,
           VerifyBuffer *verifyBuf,
           uint32_t cia,
           uint32_t instr);

} // namespace jit

} // namespace cpu
