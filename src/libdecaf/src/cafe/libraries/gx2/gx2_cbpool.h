#pragma once
#include "gx2_enum.h"
#include "gx2_internal_writegatherptr.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_pm4_sizer.h>
#include <libgpu/latte/latte_pm4_writer.h>

namespace cafe::gx2
{

using GX2Timestamp = uint64_t;

GX2Timestamp
GX2GetRetiredTimeStamp();

GX2Timestamp
GX2GetLastSubmittedTimeStamp();

void
GX2SubmitUserTimeStamp(virt_ptr<GX2Timestamp> dst,
                       GX2Timestamp timestamp,
                       GX2PipeEvent type,
                       BOOL triggerInterrupt);

BOOL
GX2WaitTimeStamp(GX2Timestamp time);

namespace internal
{

struct ActiveCommandBuffer
{
   //! Write gather address for writing to buffer.
   be2_write_gather_ptr<uint32_t> writeGatherPtr;

   //! Buffer for commands.
   be2_virt_ptr<uint32_t> buffer;

   //! Write position of buffer in words.
   be2_val<uint32_t> bufferPosWords;

   //! Size of buffer in words.
   be2_val<uint32_t> bufferSizeWords;

   //! Size of the current writing command.
   be2_val<uint32_t> cmdSize;

   //! Maximum size of the current writing command.
   be2_val<uint32_t> cmdSizeTarget;

   be2_val<BOOL> isUserBuffer;
   be2_virt_ptr<uint32_t> cbPoolBase;
   be2_val<uint32_t> cbPoolNumWords;
   be2_val<uint32_t> cbPoolNumCommandBuffers;

   UNKNOWN(0x4);
};

void
initialiseCommandBufferPool(virt_ptr<void> base,
                            uint32_t size);

virt_ptr<ActiveCommandBuffer>
getActiveCommandBuffer();

virt_ptr<ActiveCommandBuffer>
getWriteCommandBuffer(uint32_t numWords);

void
beginUserCommandBuffer(virt_ptr<uint32_t> displayList,
                       uint32_t bytes,
                       BOOL profilingEnabled);

uint32_t
endUserCommandBuffer(virt_ptr<uint32_t> displayList);

void
flushCommandBuffer(uint32_t requiredNumWords,
                   BOOL writeConfirmTimestamp);

void
queueCommandBuffer(virt_ptr<uint32_t> cbBase,
                   uint32_t cbSize,
                   virt_ptr<virt_addr> gpuLastReadPointer,
                   BOOL writeConfirmTimestamp);

void
debugCaptureCbPoolPointers();

void
debugCaptureCbPoolPointersFree();

/**
 * Write the pm4 serialiser to the given buffer
 */
template<typename Type>
inline void
writePM4(virt_ptr<uint32_t> buffer,
         uint32_t &bufferPosWords,
         const Type &command)
{
   // Remove const for the .serialise function
   auto &cmd = const_cast<Type &>(command);

   // Calculate the total size this object will be
   latte::pm4::PacketSizer sizer;
   cmd.serialise(sizer);
   auto totalSize = sizer.getSize() + 1;

   // Serialize the packet to the given buffer
   auto writer = latte::pm4::PacketWriter {
      buffer.getRawPointer(),
      bufferPosWords,
      Type::Opcode,
      totalSize
   };
   cmd.serialise(writer);
}


/**
 * Write a PM4 command to the active command buffer for the current core.
 */
template<typename Type>
inline void
writePM4(const Type &command)
{
   // Remove const for the .serialise function.
   auto &cmd = const_cast<Type &>(command);

   // Calculate the total size this command will be.
   latte::pm4::PacketSizer sizer;
   cmd.serialise(sizer);

   // Allocate a command buffer.
   auto totalSize = sizer.getSize() + 1;
   auto cmdSize = uint32_t { 0 };
   auto cb = getWriteCommandBuffer(totalSize);

   // Serialize the packet to the command buffer.
   auto writer = latte::pm4::PacketWriter {
      cb->writeGatherPtr.get().getRawPointer(),
      cmdSize,
      Type::Opcode,
      totalSize
   };
   cmd.serialise(writer);
   cb->cmdSize = cmdSize;

   // Verify the size was as expected.
   decaf_check(cb->cmdSize == cb->cmdSizeTarget);
}


inline void
writeType0(latte::Register baseIndex,
           gsl::span<uint32_t> values)
{
   auto numValues = static_cast<uint32_t>(values.size());
   auto cb = getWriteCommandBuffer(numValues + 1);
   auto header = latte::pm4::HeaderType0::get(0)
      .type(latte::pm4::PacketType::Type0)
      .baseIndex(baseIndex / 4)
      .count(numValues - 1);

   // Write data to command buffer
   *cb->writeGatherPtr = header.value;
   for (auto value : values) {
      *cb->writeGatherPtr = value;
   }

   // Verify the size was as expected.
   cb->cmdSize = numValues + 1;
   decaf_check(cb->cmdSize == cb->cmdSizeTarget);
}

inline void
writeType0(latte::Register id,
           uint32_t value)
{
   writeType0(id, { &value, 1 });
}

} // namespace internal

} // namespace cafe::gx2
