#pragma once
#include "coreinit_ios.h"
#include "coreinit_mutex.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/be_ptr.h>
#include <common/structsize.h>

namespace coreinit
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
   be_val<int32_t> pushIndex;

   //! The current message index to pop from.
   be_val<int32_t> popIndex;

   //! The number of messages in the queue.
   be_val<int32_t> count;

   //! Tracks the total number of messages in the count.
   be_val<int32_t> maxCount;

   //! Messages in the queue.
   be_ptr<be_ptr<void>> messages;
};
CHECK_OFFSET(IPCBufPoolFIFO, 0x00, pushIndex);
CHECK_OFFSET(IPCBufPoolFIFO, 0x04, popIndex);
CHECK_OFFSET(IPCBufPoolFIFO, 0x08, count);
CHECK_OFFSET(IPCBufPoolFIFO, 0x0C, maxCount);
CHECK_OFFSET(IPCBufPoolFIFO, 0x10, messages);
CHECK_SIZE(IPCBufPoolFIFO, 0x14);


/**
 * A simple message buffer pool used for IPC communication.
 */
struct IPCBufPool
{
   static constexpr uint32_t MagicHeader = 0x0BADF00D;

   //! Magic header always set to IPCBufPool::MagicHeader.
   be_val<uint32_t> magic;

   //! Pointer to buffer used for this IPCBufPool.
   be_ptr<void> buffer;

   //! Size of buffer.
   be_val<uint32_t> size;

   be_val<uint32_t> unk0x0C;
   be_val<uint32_t> unk0x10;

   //! Message size from IPCBufPoolCreate.
   be_val<uint32_t> messageSize0x14;

   //! Message size from IPCBufPoolCreate.
   be_val<uint32_t> messageSize0x18;

   //! Number of messages in the IPCBufPoolFIFO.
   be_val<uint32_t> messageCount;

   //! Pointer to start of messages.
   be_ptr<void> messages;

   //! Number of bytes used for the message pointers in IPCBufPoolFIFO.
   be_val<uint32_t> messageIndexSize;

   //! FIFO queue of messages.
   IPCBufPoolFIFO fifo;

   //! Mutex used to secure access to fifo.
   OSMutex mutex;

   UNKNOWN(0x6C - 0x68);
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

IPCBufPool *
IPCBufPoolCreate(void *buffer,
                 uint32_t size,
                 uint32_t messageSize,
                 be_val<uint32_t> *numMessagesOut,
                 uint32_t unk0x0c);

void *
IPCBufPoolAllocate(IPCBufPool *pool,
                   uint32_t size);

IOSError
IPCBufPoolFree(IPCBufPool *pool,
               void *message);

/** @} */

} // namespace coreinit
