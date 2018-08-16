#pragma once
#include "cafe/libraries/coreinit/coreinit_time.h"

#include <atomic>
#include <cstdint>
#include <libgpu/latte/latte_pm4_sizer.h>
#include <libgpu/latte/latte_pm4_writer.h>

namespace cafe::gx2::internal
{

struct CommandBuffer
{
   bool displayList = false;
   cafe::coreinit::OSTime submitTime = 0;
   uint32_t *buffer = nullptr;
   uint32_t curSize = 0;
   uint32_t maxSize = 0;
   std::atomic<CommandBuffer *> next;
};

void
initCommandBufferPool(virt_ptr<uint32_t> base,
                      uint32_t size);

CommandBuffer *
flushCommandBuffer(uint32_t neededSize);

void
freeCommandBuffer(CommandBuffer *cb);

CommandBuffer *
getCommandBuffer(uint32_t size);

void
padCommandBuffer(CommandBuffer *buffer);

void
queueDisplayList(uint32_t *buffer,
                 uint32_t size);

bool
getUserCommandBuffer(uint32_t **buffer,
                     uint32_t *maxSize);

void
beginUserCommandBuffer(uint32_t *buffer,
                       uint32_t size);

uint32_t
endUserCommandBuffer(uint32_t *buffer);

void
onRetireCommandBuffer(void *context);

/**
 * Write a PM4 command to the active command buffer.
 */
template<typename Type>
void writePM4(const Type &value)
{
   auto &ncValue = const_cast<Type &>(value);

   // Calculate the total size this object will be
   latte::pm4::PacketSizer sizer;
   ncValue.serialise(sizer);
   auto totalSize = sizer.getSize() + 1;

   // Serialize the packet to the active command buffer
   auto buffer = getCommandBuffer(totalSize);
   auto writer = latte::pm4::PacketWriter { buffer->buffer,
                                            buffer->curSize,
                                            Type::Opcode,
                                            totalSize };
   ncValue.serialise(writer);
}

} // namespace cafe::gx2::internal
