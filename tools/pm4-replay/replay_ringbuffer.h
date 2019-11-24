#pragma once
#include <libdecaf/src/cafe/cafe_tinyheap.h>
#include <libgpu/gpu_ringbuffer.h>
#include <libgpu/latte/latte_pm4_sizer.h>
#include <libgpu/latte/latte_pm4_writer.h>
#include <cstring>

class RingBuffer
{
   static constexpr auto BufferSize = 0x1000000u;

public:
   RingBuffer(phys_ptr<cafe::TinyHeapPhysical> replayHeap) :
      mBufferHeap(replayHeap)
   {
      auto allocPtr = phys_ptr<void> { nullptr };

      cafe::TinyHeap_Alloc(replayHeap, BufferSize * 4, 0x100, &allocPtr);
      mBuffer = phys_cast<uint32_t *>(allocPtr);
      mSize = BufferSize;
   }

   ~RingBuffer()
   {
      cafe::TinyHeap_Free(mBufferHeap, mBuffer);
   }

   void
   clear()
   {
      decaf_check(mWritePosition == mSubmitPosition);
      mWritePosition = 0u;
      mSubmitPosition = 0u;
   }

   void
   flushCommandBuffer()
   {
      gpu::ringbuffer::write({
         mBuffer.getRawPointer() + mSubmitPosition,
         mWritePosition - mSubmitPosition
      });

      mSubmitPosition = mWritePosition;
   }

   template<typename Type>
   void
   writePM4(const Type &value)
   {
      auto &ncValue = const_cast<Type &>(value);

      // Calculate the total size this object will be
      latte::pm4::PacketSizer sizer;
      ncValue.serialise(sizer);
      auto totalSize = sizer.getSize() + 1;

      // Serialize the packet to the active command buffer
      decaf_check(mWritePosition + totalSize < mSize);
      auto writer = latte::pm4::PacketWriter {
         mBuffer.getRawPointer(),
         mWritePosition,
         Type::Opcode,
         totalSize
      };
      ncValue.serialise(writer);
   }

   void
   writeBuffer(void *buffer, uint32_t numWords)
   {
      decaf_check(mWritePosition + numWords < mSize);
      std::memcpy(mBuffer.getRawPointer() + mWritePosition, buffer, numWords * 4);
      mWritePosition += numWords;
   }

private:
   phys_ptr<cafe::TinyHeapPhysical> mBufferHeap = nullptr;
   phys_ptr<uint32_t> mBuffer = nullptr;
   uint32_t mSize = 0u;
   uint32_t mSubmitPosition = 0u;
   uint32_t mWritePosition = 0u;
};
