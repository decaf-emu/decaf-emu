#pragma once
#include <libdecaf/src/cafe/cafe_tinyheap.h>
#include <libgpu/gpu_ringbuffer.h>
#include <libgpu/gpu_ih.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_sizer.h>
#include <libgpu/latte/latte_pm4_writer.h>

#include <atomic>
#include <cstring>
#include <mutex>
#include <condition_variable>

class RingBuffer
{
   static constexpr auto BufferSize = 0x1000000u;

public:
   RingBuffer(phys_ptr<cafe::TinyHeapPhysical> replayHeap) :
      mReplayHeap(replayHeap)
   {
      auto allocPtr = phys_ptr<void> { nullptr };

      cafe::TinyHeap_Alloc(replayHeap, 8, 0x100, &allocPtr);
      mRetireTimestampMemory = phys_cast<uint64_t *>(allocPtr);
      *mRetireTimestampMemory = 0ull;
      mRetireTimestamp = 0ull;
      mSubmitTimestamp = 0ull;
   }

   ~RingBuffer()
   {
      if (mRetireTimestampMemory) {
         cafe::TinyHeap_Free(mReplayHeap, mRetireTimestampMemory);
      }
   }

   uint64_t
   insertRetiredTimestamp()
   {
      auto submitTimestamp = ++mSubmitTimestamp;
      auto eventType = latte::VGT_EVENT_TYPE::CACHE_FLUSH_TS;
      writePM4(
         latte::pm4::EventWriteEOP {
            latte::VGT_EVENT_INITIATOR::get(0)
               .EVENT_TYPE(eventType)
               .EVENT_INDEX(latte::VGT_EVENT_INDEX::TS),
            latte::pm4::EW_ADDR_LO::get(0)
               .ADDR_LO(static_cast<uint32_t>(phys_cast<phys_addr>(mRetireTimestampMemory)) >> 2)
               .ENDIAN_SWAP(latte::CB_ENDIAN::SWAP_8IN64),
            latte::pm4::EWP_ADDR_HI::get(0)
               .DATA_SEL(latte::pm4::EWP_DATA_64)
               .INT_SEL(latte::pm4::EWP_INT_WRITE_CONFIRM),
            static_cast<uint32_t>(submitTimestamp & 0xFFFFFFFF),
            static_cast<uint32_t>(submitTimestamp >> 32)
         });
      return submitTimestamp;
   }

   uint64_t
   flushCommandBuffer()
   {
      auto timestamp = insertRetiredTimestamp();
      gpu::ringbuffer::write(mBuffer);
      mBuffer.clear();
      return timestamp;
   }

   void
   waitTimestamp(uint64_t timestamp)
   {
      std::unique_lock<std::mutex> lock{ mRetireMutex };
      mRetireCV.wait(lock, [this]() {
         return mRetireTimestamp >= mSubmitTimestamp.load();
      });
   }

   void
   onGpuInterrupt()
   {
      std::unique_lock<std::mutex> lock { mRetireMutex };
      mRetireTimestamp = *mRetireTimestampMemory;
      mRetireCV.notify_all();
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

      // Resize buffer
      auto writePosition = static_cast<uint32_t>(mBuffer.size());
      mBuffer.resize(mBuffer.size() + totalSize);

      // Serialize the packet to the active command buffer
      auto writer = latte::pm4::PacketWriter {
         mBuffer.data(),
         writePosition,
         Type::Opcode,
         totalSize
      };
      ncValue.serialise(writer);
      decaf_check(writePosition == mBuffer.size());
   }

   void
   writeBuffer(const void *buffer, uint32_t numWords)
   {
      auto offset = mBuffer.size();
      mBuffer.resize(mBuffer.size() + numWords);
      std::memcpy(mBuffer.data() + offset, buffer, numWords * 4);
   }

private:
   std::vector<uint32_t> mBuffer;

   phys_ptr<cafe::TinyHeapPhysical> mReplayHeap = nullptr;
   phys_ptr<uint64_t> mRetireTimestampMemory = nullptr;

   std::atomic<uint64_t> mSubmitTimestamp = 0ull;
   std::atomic<uint64_t> mRetireTimestamp = 0ull;

   std::mutex mRetireMutex;
   std::condition_variable mRetireCV;
};
