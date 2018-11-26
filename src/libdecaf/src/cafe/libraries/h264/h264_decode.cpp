#include "h264.h"
#include "h264_decode.h"
#include "h264_decode_ffmpeg.h"
#include "h264_decode_null.h"
#include "h264_stream.h"

#include "cafe/libraries/cafe_hle_stub.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

namespace cafe::h264
{

constexpr auto BaseMemoryRequirement = 0x480000u;
constexpr auto WorkMemoryAlign = 0x100u;

constexpr auto MinimumWidth = 64u;
constexpr auto MinimumHeight = 64u;
constexpr auto MaximumWidth = 2800u;
constexpr auto MaximumHeight = 1408u;

namespace internal
{

virt_ptr<H264WorkMemory>
getWorkMemory(virt_ptr<void> memory)
{
   auto workMemory = byte_swap<virt_addr>(*virt_cast<virt_addr *>(memory));
   auto addrDiff = workMemory - virt_cast<virt_addr>(memory);
   if (addrDiff > WorkMemoryAlign || addrDiff < 0) {
      return nullptr;
   }

   return virt_cast<H264WorkMemory *>(workMemory);
}

static void
initialiseWorkMemory(virt_ptr<H264WorkMemory> workMemory,
                     virt_addr alignedMemoryEnd)
{
   auto baseAddress = virt_cast<virt_addr>(workMemory);
   workMemory->streamMemory = virt_cast<H264StreamMemory *>(baseAddress + 0x100u);
   workMemory->unk0x10 = virt_cast<void *>(baseAddress + 0x439C00);
   workMemory->bitStream = virt_cast<H264Bitstream *>(baseAddress + 0x439D00);
   workMemory->database = virt_cast<void *>(baseAddress + 0x439E00);
   workMemory->codecMemory = virt_cast<H264CodecMemory *>(baseAddress + 0x442300);
   workMemory->l1Memory = virt_cast<void *>(baseAddress + 0x445F00);
   workMemory->unk0x24 = virt_cast<void *>(baseAddress + 0x447F00);
   workMemory->internalFrameMemory = virt_cast<void *>(baseAddress + 0x44FC00);

   workMemory->streamMemory->workMemoryEnd =
      virt_cast<void *>(alignedMemoryEnd);
   workMemory->streamMemory->unkMemory =
      virt_cast<void *>(alignedMemoryEnd - 0x450000 + 0x300);
}

static uint32_t
getLevelMemoryRequirement(int32_t level)
{
   switch (level) {
   case 10:
      return 0x63000;
   case 11:
      return 0xE1000;
   case 12:
   case 13:
   case 20:
      return 0x252000;
   case 21:
      return 0x4A4000;
   case 22:
   case 30:
      return 0x7E9000;
   case 31:
      return 0x1194000;
   case 32:
      return 0x1400000;
   case 40:
   case 41:
      return 0x2000000;
   case 42:
      return 0x2200000;
   case 50:
      return 0x6BD0000;
   case 51:
      return 0xB400000;
   default:
      decaf_abort(fmt::format("Unexpected H264 level {}", level));
   }
}

} // namespace internal


/**
 * Calculate the amount of memory required for the specified parameters.
 */
H264Error
H264DECMemoryRequirement(int32_t profile,
                         int32_t level,
                         int32_t maxWidth,
                         int32_t maxHeight,
                         virt_ptr<uint32_t> outMemoryRequirement)
{
   if (maxWidth < MinimumWidth || maxHeight < MinimumHeight ||
       maxWidth > MaximumWidth || maxHeight > MaximumHeight) {
      return H264Error::InvalidParameter;
   }

   if (!outMemoryRequirement) {
      return H264Error::InvalidParameter;
   }

   if (level > 51) {
      return H264Error::InvalidParameter;
   }

   if (profile != 66 && profile != 77 && profile != 100) {
      return H264Error::InvalidParameter;
   }

   *outMemoryRequirement =
      BaseMemoryRequirement + internal::getLevelMemoryRequirement(level) + 1023u;
   return H264Error::OK;
}


/**
 * Initialise a H264 decoder in the given memory.
 */
H264Error
H264DECInitParam(int32_t memorySize,
                 virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   if (memorySize < 0x44F900) {
      return H264Error::OutOfMemory;
   }

   std::memset(memory.get(), 0, memorySize);

   // Calculate aligned memory
   auto alignedMemory = align_up(memory, WorkMemoryAlign);
   auto alignOffset =
      virt_cast<virt_addr>(memory) - virt_cast<virt_addr>(alignedMemory);

   auto alignedMemoryStart = virt_cast<virt_addr>(alignedMemory);
   auto alignedMemoryEnd = alignedMemoryStart + memorySize - alignOffset;

   // Write aligned memory start reversed to memory
   *virt_cast<virt_addr *>(memory) = byte_swap(alignedMemoryStart);

   // Initialise our memory
   auto workMemory = virt_cast<H264WorkMemory *>(alignedMemoryStart);
   internal::initialiseWorkMemory(workMemory, alignedMemoryEnd);

   // TODO: More things.
   return H264Error::OK;
}


/**
 * Set H264 decoder parameter.
 */
H264Error
H264DECSetParam(virt_ptr<void> memory,
                H264Parameter parameter,
                virt_ptr<void> value)
{
   if (!memory || !value) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   auto streamMemory = workMemory->streamMemory;

   switch (parameter) {
   case H264Parameter::FramePointerOutput:
      streamMemory->paramFramePointerOutput =
         virt_func_cast<H264DECFptrOutputFn>(virt_cast<virt_addr>(value));
      break;
   case H264Parameter::OutputPerFrame:
      streamMemory->paramOutputPerFrame = *virt_cast<uint8_t *>(value);
      break;
   case H264Parameter::UserMemory:
      streamMemory->paramUserMemory = value;
      break;
   case H264Parameter::Unknown0x20000030:
      streamMemory->param_0x20000030 = *virt_cast<uint32_t *>(value);
      break;
   case H264Parameter::Unknown0x20000040:
      streamMemory->param_0x20000040 = *virt_cast<uint32_t *>(value);
      break;
   case H264Parameter::Unknown0x20000010:
      // TODO
   default:
      return H264Error::InvalidParameter;
   }

   return H264Error::OK;
}


/**
 * Set the callback which is called when a frame is output from the decoder.
 */
H264Error
H264DECSetParam_FPTR_OUTPUT(virt_ptr<void> memory,
                            H264DECFptrOutputFn value)
{
   if (!memory || !value) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   workMemory->streamMemory->paramFramePointerOutput = value;
   return H264Error::OK;
}


/**
 * Set whether the decoder should internally buffer frames or call the callback
 * immediately as soon as a frame is emitted.
 */
H264Error
H264DECSetParam_OUTPUT_PER_FRAME(virt_ptr<void> memory,
                                 uint32_t value)
{
   if (!memory || !value) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   if (value) {
      workMemory->streamMemory->paramOutputPerFrame = uint8_t { 1 };
   } else {
      workMemory->streamMemory->paramOutputPerFrame = uint8_t { 0 };
   }

   return H264Error::OK;
}


/**
 * Set a user memory pointer which is passed to the frame output callback.
 */
H264Error
H264DECSetParam_USER_MEMORY(virt_ptr<void> memory,
                            virt_ptr<void> value)
{
   if (!memory || !value) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   workMemory->streamMemory->paramUserMemory = value;
   return H264Error::OK;
}

H264Error
H264DECCheckMemSegmentation(virt_ptr<void> memory,
                            uint32_t size)
{
   if (!memory || !size) {
      return H264Error::InvalidParameter;
   }

   return H264Error::OK;
}

H264Error
H264DECOpen(virt_ptr<void> memory)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECOpen(memory);
#else
   return null::H264DECOpen(memory);
#endif
}

H264Error
H264DECBegin(virt_ptr<void> memory)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECBegin(memory);
#else
   return null::H264DECBegin(memory);
#endif
}

H264Error
H264DECSetBitstream(virt_ptr<void> memory,
                    virt_ptr<uint8_t> buffer,
                    uint32_t bufferLength,
                    double timestamp)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECSetBitstream(memory, buffer, bufferLength, timestamp);
#else
   return null::H264DECSetBitstream(memory, buffer, bufferLength, timestamp);
#endif
}

H264Error
H264DECExecute(virt_ptr<void> memory,
               virt_ptr<void> frameBuffer)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECExecute(memory, frameBuffer);
#else
   return null::H264DECExecute(memory, frameBuffer);
#endif
}

H264Error
H264DECFlush(virt_ptr<void> memory)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECFlush(memory);
#else
   return null::H264DECFlush(memory);
#endif
}

H264Error
H264DECEnd(virt_ptr<void> memory)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECEnd(memory);
#else
   return null::H264DECEnd(memory);
#endif
}

H264Error
H264DECClose(virt_ptr<void> memory)
{
#ifdef DECAF_FFMPEG
   return ffmpeg::H264DECClose(memory);
#else
   return null::H264DECClose(memory);
#endif
}

void
Library::registerDecodeSymbols()
{
   RegisterFunctionExport(H264DECMemoryRequirement);
   RegisterFunctionExport(H264DECInitParam);
   RegisterFunctionExport(H264DECSetParam);
   RegisterFunctionExport(H264DECSetParam_FPTR_OUTPUT);
   RegisterFunctionExport(H264DECSetParam_OUTPUT_PER_FRAME);
   RegisterFunctionExport(H264DECSetParam_USER_MEMORY);
   RegisterFunctionExport(H264DECCheckMemSegmentation);

   RegisterFunctionExport(H264DECOpen);
   RegisterFunctionExport(H264DECBegin);
   RegisterFunctionExport(H264DECSetBitstream);
   RegisterFunctionExport(H264DECExecute);
   RegisterFunctionExport(H264DECFlush);
   RegisterFunctionExport(H264DECClose);
   RegisterFunctionExport(H264DECEnd);
}

} // namespace cafe::h264
