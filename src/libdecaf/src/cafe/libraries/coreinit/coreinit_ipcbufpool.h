#pragma once
#include "coreinit_ios.h"
#include "coreinit_mutex.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_ipcbufpool IPC Buffer Pool
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

/**
 * FIFO queue for IPCBufPool.
 *
 * Functions similar to a ring buffer.
 */
struct IPCBufPoolFIFO
{
   //! The current message index to push to.
   be2_val<int32_t> pushIndex;

   //! The current message index to pop from.
   be2_val<int32_t> popIndex;

   //! The number of messages in the queue.
   be2_val<int32_t> count;

   //! Tracks the total number of messages in the count.
   be2_val<int32_t> maxCount;

   //! Messages in the queue.
   be2_virt_ptr<virt_ptr<void>> messages;
};
CHECK_OFFSET(IPCBufPoolFIFO, 0x00, pushIndex);
CHECK_OFFSET(IPCBufPoolFIFO, 0x04, popIndex);
CHECK_OFFSET(IPCBufPoolFIFO, 0x08, count);
CHECK_OFFSET(IPCBufPoolFIFO, 0x0C, maxCount);
CHECK_OFFSET(IPCBufPoolFIFO, 0x10, messages);
CHECK_SIZE(IPCBufPoolFIFO, 0x14);


/**
 * Attributes returned by IPCBufPoolGetAttributes.
 */
struct IPCBufPoolAttributes
{
   //! Size of a message in the buffer pool.
   be2_val<uint32_t> messageSize;

   //! Size of the buffer pool.
   be2_val<uint32_t> poolSize;

   //! Number of pending messages in the pool fifo.
   be2_val<uint32_t> numMessages;
};
CHECK_OFFSET(IPCBufPoolAttributes, 0x00, messageSize);
CHECK_OFFSET(IPCBufPoolAttributes, 0x04, poolSize);
CHECK_OFFSET(IPCBufPoolAttributes, 0x08, numMessages);
CHECK_SIZE(IPCBufPoolAttributes, 0x0C);


/**
 * A simple message buffer pool used for IPC communication.
 */
struct IPCBufPool
{
   static constexpr uint32_t MagicHeader = 0x0BADF00Du;

   //! Magic header always set to IPCBufPool::MagicHeader.
   be2_val<uint32_t> magic;

   //! Pointer to buffer used for this IPCBufPool.
   be2_virt_ptr<void> buffer;

   //! Size of buffer.
   be2_val<uint32_t> size;

   be2_val<uint32_t> unk0x0C;
   be2_val<uint32_t> unk0x10;

   //! Message size from IPCBufPoolCreate.
   be2_val<uint32_t> messageSize0x14;

   //! Message size from IPCBufPoolCreate.
   be2_val<uint32_t> messageSize0x18;

   //! Number of messages in the IPCBufPoolFIFO.
   be2_val<uint32_t> messageCount;

   //! Pointer to start of messages.
   be2_virt_ptr<void> messages;

   //! Number of bytes used for the message pointers in IPCBufPoolFIFO.
   be2_val<uint32_t> messageIndexSize;

   //! FIFO queue of messages.
   be2_struct<IPCBufPoolFIFO> fifo;

   //! Mutex used to secure access to fifo.
   be2_struct<OSMutex> mutex;

   UNKNOWN(0x4);
};
CHECK_OFFSET(IPCBufPool, 0x00, magic);
CHECK_OFFSET(IPCBufPool, 0x04, buffer);
CHECK_OFFSET(IPCBufPool, 0x08, size);
CHECK_OFFSET(IPCBufPool, 0x0C, unk0x0C);
CHECK_OFFSET(IPCBufPool, 0x10, unk0x10);
CHECK_OFFSET(IPCBufPool, 0x14, messageSize0x14);
CHECK_OFFSET(IPCBufPool, 0x18, messageSize0x18);
CHECK_OFFSET(IPCBufPool, 0x1C, messageCount);
CHECK_OFFSET(IPCBufPool, 0x20, messages);
CHECK_OFFSET(IPCBufPool, 0x24, messageIndexSize);
CHECK_OFFSET(IPCBufPool, 0x28, fifo);
CHECK_OFFSET(IPCBufPool, 0x3C, mutex);
CHECK_SIZE(IPCBufPool, 0x6C);

#pragma pack(pop)

virt_ptr<IPCBufPool>
IPCBufPoolCreate(virt_ptr<void> buffer,
                 uint32_t size,
                 uint32_t messageSize,
                 virt_ptr<uint32_t> outNumMessages,
                 uint32_t unk0x0c);

virt_ptr<void>
IPCBufPoolAllocate(virt_ptr<IPCBufPool> pool,
                   uint32_t size);

IOSError
IPCBufPoolFree(virt_ptr<IPCBufPool> pool,
               virt_ptr<void> message);

IOSError
IPCBufPoolGetAttributes(virt_ptr<IPCBufPool> pool,
                        virt_ptr<IPCBufPoolAttributes> attribs);

/** @} */

} // namespace cafe::coreinit
