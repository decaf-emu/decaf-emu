#pragma once
#include "h264_enum.h"
#include "h264_stream.h"

#include <libcpu/be2_struct.h>

namespace cafe::h264
{

struct H264Bitstream;
struct H264CodecMemory;
struct H264StreamMemory;

struct H264WorkMemory
{
   //! Space for pointer to aligned memory in case the original memory was
   //! already aligned.
   PADDING(4);

   //! End of the h264 work memory
   be2_virt_ptr<void> workMemoryEnd;
   be2_virt_ptr<void> unk0x08;
   be2_virt_ptr<H264StreamMemory> streamMemory;
   be2_virt_ptr<void> unk0x10;
   be2_virt_ptr<H264Bitstream> bitStream;
   be2_virt_ptr<void> database;
   be2_virt_ptr<H264CodecMemory> codecMemory;
   be2_virt_ptr<void> l1Memory;
   be2_virt_ptr<void> unk0x24;
   be2_virt_ptr<void> internalFrameMemory;
};
CHECK_OFFSET(H264WorkMemory, 0x04, workMemoryEnd);
CHECK_OFFSET(H264WorkMemory, 0x08, unk0x08);
CHECK_OFFSET(H264WorkMemory, 0x0C, streamMemory);
CHECK_OFFSET(H264WorkMemory, 0x10, unk0x10);
CHECK_OFFSET(H264WorkMemory, 0x14, bitStream);
CHECK_OFFSET(H264WorkMemory, 0x18, database);
CHECK_OFFSET(H264WorkMemory, 0x1C, codecMemory);
CHECK_OFFSET(H264WorkMemory, 0x20, l1Memory);
CHECK_OFFSET(H264WorkMemory, 0x24, unk0x24);
CHECK_OFFSET(H264WorkMemory, 0x28, internalFrameMemory);
CHECK_SIZE(H264WorkMemory, 0x2C);

H264Error
H264DECMemoryRequirement(int32_t profile,
                         int32_t level,
                         int32_t maxWidth,
                         int32_t maxHeight,
                         virt_ptr<uint32_t> outMemoryRequirement);

H264Error
H264DECInitParam(int32_t memorySize,
                 virt_ptr<void> memory);

H264Error
H264DECSetParam(virt_ptr<void> memory,
                H264Parameter parameter,
                virt_ptr<void> value);

H264Error
H264DECSetParam_FPTR_OUTPUT(virt_ptr<void> memory,
                            H264DECFptrOutputFn value);

H264Error
H264DECSetParam_OUTPUT_PER_FRAME(virt_ptr<void> memory,
                                 uint32_t value);

H264Error
H264DECSetParam_USER_MEMORY(virt_ptr<void> memory,
                            virt_ptr<void> value);

H264Error
H264DECCheckMemSegmentation(virt_ptr<void> memory,
                            uint32_t size);

H264Error
H264DECOpen(virt_ptr<void> memory);

H264Error
H264DECBegin(virt_ptr<void> memory);

H264Error
H264DECSetBitstream(virt_ptr<void> memory,
                    virt_ptr<uint8_t> buffer,
                    uint32_t bufferLength,
                    double timestamp);

H264Error
H264DECExecute(virt_ptr<void> memory,
               virt_ptr<void> frameBuffer);

H264Error
H264DECFlush(virt_ptr<void> memory);

H264Error
H264DECEnd(virt_ptr<void> memory);

H264Error
H264DECClose(virt_ptr<void> memory);

namespace internal
{

virt_ptr<H264WorkMemory>
getWorkMemory(virt_ptr<void> memory);

} // namespace internal

} // namespace cafe::h264
