
/*

r1 + 0x30 = 0
r1 + 0x34 = dlist + numWords * 4
r1 + 0x38 = 0x4 00 00 (DATA32 = 1) ?
r1 + 0x3C = OSEffectiveToPhysical(queue cb r5) | 2
r1 + 0x40 = 0xC0 03 3D 00 (MEM_WRITE)

r1 + 0x44 = numWords
r1 + 0x48 = 0
r1 + 0x4C = OSEffectiveToPhysical(dlist)
r1 + 0x50 = 0xC0 02 32 00 (INDIRECT_BUFFER_PRIV)

TCLSubmitToRing(r3 -> INDIRECT_BUFFER_PRIV + MEM_WRITE,
                r4 = 9 length,
                r5 = 0x20000000,
                r6 = ptr to LastSubmittedTimestamp)


start = 0x8(r27) ??
end = 0xc(r27) ??
GX2GetCBTimestampData() = return 0xC(r27) << 2 + GX2GetCBBasePtr()

GX2GetCBBasePtr() = return 0x1C(r27) + 0x28(r27) << 16;

__OSSetInterruptHandler(2, .... ) TCLInterrupt = 2 ?
*/

#include <cstdint>

namespace tcl
{

enum TCLStatus
{
   OK = 0,
   Timeout = 0x16,
};

enum TCLIHEvent
{
   RetiredInterrupt = 0xB5,
};

TCLStatus
TCLSubmitToRing(void *pm4Buffer, uint32_t numWords, uint32_t unk, uint64_t *lastSubmittedTimestamp)
{
   *lastSubmittedTimestamp += 1;
   return TCLStatus::OK;
}

} // namespace tcl
