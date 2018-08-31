#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mutex.h"

#include <common/align.h>

namespace cafe::coreinit
{

constexpr uint32_t IPCBufPool::MagicHeader;

namespace internal
{

static void
ipcBufPoolFifoInit(virt_ptr<IPCBufPoolFIFO> fifo,
                   int32_t numMessages,
                   virt_ptr<virt_ptr<void>> messages);

static IOSError
ipcBufPoolFifoPush(virt_ptr<IPCBufPoolFIFO> fifo,
                   virt_ptr<void> message);

static IOSError
ipcBufPoolFifoPop(virt_ptr<IPCBufPoolFIFO> fifo,
                  virt_ptr<void> *outMessage);

static int32_t
ipcBufPoolGetMessageIndex(virt_ptr<IPCBufPool> pool,
                          virt_ptr<void> message);

} // namespace internal


/**
 * Create a IPCBufPool structure from buffer.
 */
virt_ptr<IPCBufPool>
IPCBufPoolCreate(virt_ptr<void> buffer,
                 uint32_t size,
                 uint32_t messageSize,
                 virt_ptr<uint32_t> outNumMessages,
                 uint32_t unk0x0c)
{
   if (!buffer || size == 0 || messageSize == 0) {
      return nullptr;
   }

   std::memset(buffer.getRawPointer(), 0, size);

   // IPC messages should be 64 byte aligned
   messageSize = align_up(messageSize, 64);

   auto alignedBuffer = align_up(buffer, 0x40);
   auto pool = virt_cast<IPCBufPool *>(alignedBuffer);
   pool->magic = IPCBufPool::MagicHeader;
   pool->buffer = buffer;
   pool->size = size;
   pool->unk0x0C = unk0x0c;
   pool->unk0x10 = 0u;
   pool->messageSize0x14 = messageSize;
   pool->messageSize0x18 = messageSize;
   OSInitMutexEx(virt_addrof(pool->mutex), nullptr);

   auto numMessages = static_cast<uint32_t>((size - sizeof(IPCBufPool)) / messageSize);

   if (numMessages <= 1) {
      return nullptr;
   }

   auto messageIndexSize = static_cast<uint32_t>(numMessages * sizeof(be2_virt_ptr<void>));
   numMessages -= 1 + (messageIndexSize / messageSize);
   *outNumMessages = numMessages;
   pool->messageCount = numMessages;

   auto messageIndex = virt_cast<virt_ptr<void> *>(virt_cast<uint8_t *>(alignedBuffer) + sizeof(IPCBufPool));
   auto messages = virt_cast<uint8_t *>(messageIndex) + messageIndexSize;
   pool->messages = messageIndex;
   pool->messageIndexSize = messageIndexSize;

   // Initialise FIFO.
   internal::ipcBufPoolFifoInit(virt_addrof(pool->fifo),
                                static_cast<int32_t>(numMessages),
                                messageIndex);

   for (auto i = 0u; i < numMessages; ++i) {
      internal::ipcBufPoolFifoPush(virt_addrof(pool->fifo), messages + i * messageSize);
   }

   return pool;
}


/**
 * Allocate a message from an IPCBufPool.
 */
virt_ptr<void>
IPCBufPoolAllocate(virt_ptr<IPCBufPool> pool,
                   uint32_t size)
{
   if (pool->magic != IPCBufPool::MagicHeader) {
      return nullptr;
   }

   if (size > pool->messageSize0x14) {
      return nullptr;
   }

   auto message = virt_ptr<void> { nullptr };
   OSLockMutex(virt_addrof(pool->mutex));
   internal::ipcBufPoolFifoPop(virt_addrof(pool->fifo), &message);
   OSUnlockMutex(virt_addrof(pool->mutex));
   return message;
}


/**
 * Free a message back to a IPCBufPool.
 */
IOSError
IPCBufPoolFree(virt_ptr<IPCBufPool> pool,
               virt_ptr<void> message)
{
   auto error = IOSError::OK;

   if (pool->magic != IPCBufPool::MagicHeader) {
      return IOSError::Invalid;
   }

   OSLockMutex(virt_addrof(pool->mutex));
   auto index = internal::ipcBufPoolGetMessageIndex(pool, message);

   if (index >= 0) {
      auto messages = virt_cast<uint8_t *>(pool->messages);
      auto message = messages + index * pool->messageSize0x18;
      internal::ipcBufPoolFifoPush(virt_addrof(pool->fifo), message);
   } else {
      error = IOSError::Invalid;
   }

   OSUnlockMutex(virt_addrof(pool->mutex));
   return error;
}


/**
 * Get some information about an IPCBufPool object.
 */
IOSError
IPCBufPoolGetAttributes(virt_ptr<IPCBufPool> pool,
                        virt_ptr<IPCBufPoolAttributes> attribs)
{
   if (pool->magic != IPCBufPool::MagicHeader) {
      return IOSError::Invalid;
   }

   OSLockMutex(virt_addrof(pool->mutex));
   attribs->messageSize = pool->messageSize0x14;
   attribs->poolSize = pool->messageCount;
   attribs->numMessages = pool->fifo.count;
   OSUnlockMutex(virt_addrof(pool->mutex));
   return IOSError::OK;
}


namespace internal
{

/**
 * Initialise an IPCBufPoolFIFO structure.
 */
void
ipcBufPoolFifoInit(virt_ptr<IPCBufPoolFIFO> fifo,
                   int32_t numMessages,
                   virt_ptr<virt_ptr<void>> messages)
{
   fifo->pushIndex = 0;
   fifo->popIndex = -1;
   fifo->count = 0;
   fifo->maxCount = numMessages;
   fifo->messages = messages;
}


/**
 * Push a message into a IPCBufPoolFIFO structure.
 */
IOSError
ipcBufPoolFifoPush(virt_ptr<IPCBufPoolFIFO> fifo,
                   virt_ptr<void> message)
{
   if (fifo->pushIndex == fifo->popIndex) {
      return IOSError::QFull;
   }

   fifo->messages[fifo->pushIndex] = message;

   if (fifo->popIndex == -1) {
      fifo->popIndex = fifo->pushIndex;
   }

   fifo->count += 1;
   fifo->pushIndex = (fifo->pushIndex + 1) % fifo->maxCount;
   return IOSError::OK;
}


/**
 * Pop a message from a IPCBufPoolFIFO structure.
 */
IOSError
ipcBufPoolFifoPop(virt_ptr<IPCBufPoolFIFO> fifo,
                  virt_ptr<void> *outMessage)
{
   if (fifo->popIndex == -1) {
      return IOSError::QEmpty;
   }

   auto message = fifo->messages[fifo->popIndex];
   fifo->count -= 1;

   if (fifo->count == 0) {
      fifo->popIndex = -1;
   } else {
      fifo->popIndex = (fifo->popIndex + 1) % fifo->maxCount;
   }

   *outMessage = message;
   return IOSError::OK;
}


/**
 * Get the index of a message in an IPCBufPool by it's pointer.
 */
int32_t
ipcBufPoolGetMessageIndex(virt_ptr<IPCBufPool> pool,
                          virt_ptr<void> message)
{
   if (message < pool->messages) {
      return -1;
   }

   auto offset = virt_cast<virt_addr>(message) - virt_cast<virt_addr>(pool->messages);
   auto index = offset / pool->messageSize0x18;

   if (index > pool->messageCount) {
      return -1;
   }

   return static_cast<int32_t>(index);
}

} // namespace internal

void
Library::registerIpcBufPoolSymbols()
{
   RegisterFunctionExport(IPCBufPoolCreate);
   RegisterFunctionExport(IPCBufPoolAllocate);
   RegisterFunctionExport(IPCBufPoolFree);
   RegisterFunctionExport(IPCBufPoolGetAttributes);
}

} // namespace cafe::coreinit
