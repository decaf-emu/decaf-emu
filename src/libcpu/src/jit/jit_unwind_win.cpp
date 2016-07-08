#include "common/emuassert.h"
#include "common/platform.h"
#include "jit_vmemruntime.h"

#ifdef PLATFORM_WINDOWS

#include <Windows.h>

#define UBYTE uint8_t

// http://msdn.microsoft.com/en-us/library/ssa62fwe.aspx
typedef enum _UNWIND_REGISTER_CODES {
   UWRC_RAX = 0,
   UWRC_RCX = 1,
   UWRC_RDX = 2,
   UWRC_RBX = 3,
   UWRC_RSP = 4,
   UWRC_RBP = 5,
   UWRC_RSI = 6,
   UWRC_RDI = 7,
   UWRC_R8 = 8,
   UWRC_R9 = 9,
   UWRC_R10 = 10,
   UWRC_R11 = 11,
   UWRC_R12 = 12,
   UWRC_R13 = 13,
   UWRC_R14 = 14,
   UWRC_R15 = 15,
} UNWIND_REGISTER_CODES;

typedef enum _UNWIND_OP_CODES {
   UWOP_PUSH_NONVOL = 0, /* info == register number */
   UWOP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
   UWOP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
   UWOP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
   UWOP_SAVE_NONVOL,     /* info == register number, offset in next slot */
   UWOP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
   UWOP_SAVE_XMM128,     /* info == XMM reg number, offset in next slot */
   UWOP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
   UWOP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
} UNWIND_CODE_OPS;

typedef union _UNWIND_CODE {
   struct {
      UBYTE CodeOffset;
      UBYTE UnwindOp : 4;
      UBYTE OpInfo : 4;
   };
   USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

typedef struct _UNWIND_INFO {
   uint8_t Version : 3;
   uint8_t Flags : 5;
   uint8_t SizeOfProlog;
   uint8_t CountOfCodes;
   uint8_t FrameRegister : 4;
   uint8_t FrameOffset : 4;
   UNWIND_CODE UnwindCode[1];
   /*  UNWIND_CODE MoreUnwindCode[((CountOfCodes + 1) & ~1) - 1];
   *   union {
   *       OPTIONAL ULONG ExceptionHandler;
   *       OPTIONAL ULONG FunctionEntry;
   *   };
   *   OPTIONAL ULONG ExceptionData[]; */
} UNWIND_INFO, *PUNWIND_INFO;

namespace cpu
{

namespace jit
{

RUNTIME_FUNCTION *
sFunctionTable = nullptr;

void registerUnwindTable(VMemRuntime *runtime, intptr_t jitCallAddr)
{
   emuassert(!sFunctionTable);

   // This function assumes the following prologue for the jit intro:
   //   PUSH RBX
   //   PUSH ZDI
   //   PUSH ZSI
   //   PUSH R12
   //   SUB RSP, 0x38

   auto unwindCodeCount = 9;
   auto unwindInfoSize = sizeof(UNWIND_INFO) + ((unwindCodeCount + 1) & ~0x1) - 1;

   UNWIND_INFO *unwindInfo = reinterpret_cast<UNWIND_INFO*>(runtime->allocate(unwindInfoSize, 8));
   RUNTIME_FUNCTION *rfuncs = static_cast<RUNTIME_FUNCTION*>(runtime->allocate(sizeof(RUNTIME_FUNCTION) * 1, 8));

   unwindInfo->Version = 1;
   unwindInfo->Flags = 0;
   unwindInfo->SizeOfProlog = 16;
   unwindInfo->CountOfCodes = unwindCodeCount;
   unwindInfo->FrameRegister = 0;
   unwindInfo->FrameOffset = 0;
   unwindInfo->UnwindCode[0].CodeOffset = 16;
   unwindInfo->UnwindCode[0].UnwindOp = UWOP_ALLOC_SMALL;
   unwindInfo->UnwindCode[0].OpInfo = 6;
   unwindInfo->UnwindCode[1].CodeOffset = 12;
   unwindInfo->UnwindCode[1].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[1].OpInfo = UWRC_R15;
   unwindInfo->UnwindCode[2].CodeOffset = 10;
   unwindInfo->UnwindCode[2].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[2].OpInfo = UWRC_R14;
   unwindInfo->UnwindCode[3].CodeOffset = 8;
   unwindInfo->UnwindCode[3].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[3].OpInfo = UWRC_R13;
   unwindInfo->UnwindCode[4].CodeOffset = 6;
   unwindInfo->UnwindCode[4].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[4].OpInfo = UWRC_R12;
   unwindInfo->UnwindCode[5].CodeOffset = 4;
   unwindInfo->UnwindCode[5].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[5].OpInfo = UWRC_RSI;
   unwindInfo->UnwindCode[6].CodeOffset = 3;
   unwindInfo->UnwindCode[6].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[6].OpInfo = UWRC_RDI;
   unwindInfo->UnwindCode[7].CodeOffset = 2;
   unwindInfo->UnwindCode[7].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[7].OpInfo = UWRC_RBX;
   unwindInfo->UnwindCode[8].CodeOffset = 1;
   unwindInfo->UnwindCode[8].UnwindOp = UWOP_PUSH_NONVOL;
   unwindInfo->UnwindCode[8].OpInfo = UWRC_RBP;

   auto rootAddress = runtime->getRootAddress();
   rfuncs[0].BeginAddress = static_cast<DWORD>(jitCallAddr - rootAddress);
   rfuncs[0].EndAddress = static_cast<DWORD>(runtime->_sizeLimit);
   rfuncs[0].UnwindData = static_cast<DWORD>(reinterpret_cast<intptr_t>(unwindInfo) - rootAddress);

   RtlAddFunctionTable(rfuncs, 1, rootAddress);
   sFunctionTable = rfuncs;
}

void unregisterUnwindTable()
{
   RtlDeleteFunctionTable(sFunctionTable);
   sFunctionTable = nullptr;
}

} // namespace jit

} // namespace cpu

#endif // PLATFORM_WINDOWS
