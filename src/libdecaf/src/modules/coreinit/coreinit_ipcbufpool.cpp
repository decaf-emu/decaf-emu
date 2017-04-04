#include "coreinit.h"
#include "coreinit_ios.h"
#include "coreinit_ipcbufpool.h"
#include "coreinit_mutex.h"

#include <common/align.h>

namespace coreinit
{

constexpr uint32_t IPCBufPool::MagicHeader;

namespace internal
{

static void
ipcBufPoolFifoInit(IPCBufPoolFIFO *fifo,
                   uint32_t numMessages,
                   be_ptr<void> *messages);

static IOSError
ipcBufPoolFifoPush(IPCBufPoolFIFO *fifo,
                   void *message);

static IOSError
ipcBufPoolFifoPop(IPCBufPoolFIFO *fifo,
                  void **messageOut);

static int32_t
ipcBufPoolGetMessageIndex(IPCBufPool *pool,
                          void *message);

} // namespace internal


/**
 * Create a IPCBufPool structure from buffer.
 */
IPCBufPool *
IPCBufPoolCreate(void *buffer,
                 uint32_t size,
                 uint32_t messageSize,
                 be_val<uint32_t> *numMessagesOut,
                 uint32_t unk0x0c)
{
   if (!buffer || size == 0 || messageSize == 0) {
      return nullptr;
   }

   std::memset(buffer, 0, size);

   // IPC messages should be 64 byte aligned
   messageSize = align_up(messageSize, 64);

   auto pool = reinterpret_cast<IPCBufPool *>(align_up(buffer, 0x40));
   pool->magic = IPCBufPool::MagicHeader;
   pool->buffer = buffer;
   pool->size = size;
   pool->unk0x0C = unk0x0c;
   pool->unk0x10 = 0;
   pool->messageSize0x14 = messageSize;
   pool->messageSize0x18 = messageSize;
   OSInitMutexEx(&pool->mutex, nullptr);

   auto numMessages = static_cast<uint32_t>((size - sizeof(IPCBufPool)) / messageSize);

   if (numMessages <= 1) {
      return nullptr;
   }

   auto messageIndexSize = static_cast<uint32_t>(numMessages * sizeof(be_ptr<void>));
   numMessages -= 1 + (messageIndexSize / messageSize);
   *numMessagesOut = numMessages;
   pool->messageCount = numMessages;

   auto messageIndex = reinterpret_cast<be_ptr<void> *>(reinterpret_cast<uint8_t *>(pool) + sizeof(IPCBufPool));
   auto messages = reinterpret_cast<uint8_t *>(messageIndex) + messageIndexSize;
   pool->messages = messageIndex;
   pool->messageIndexSize = messageIndexSize;

   // Initialise FIFO.
   internal::ipcBufPoolFifoInit(&pool->fifo, numMessages, messageIndex);

   for (auto i = 0u; i < numMessages; ++i) {
      internal::ipcBufPoolFifoPush(&pool->fifo, messages + i * messageSize);
   }

   return pool;
}


/**
 * Allocate a message from an IPCBufPool.
 */
void *
IPCBufPoolAllocate(IPCBufPool *pool,
                   uint32_t size)
{
   if (pool->magic != IPCBufPool::MagicHeader) {
      return nullptr;
   }

   if (size > pool->messageSize0x14) {
      return nullptr;
   }

   void *message = nullptr;
   OSLockMutex(&pool->mutex);
   internal::ipcBufPoolFifoPop(&pool->fifo, &message);
   OSUnlockMutex(&pool->mutex);
   return message;
}


/**
 * Free a message back to a IPCBufPool.
 */
IOSError
IPCBufPoolFree(IPCBufPool *pool,
               void *message)
{
   auto error = IOSError::OK;

   if (pool->magic != IPCBufPool::MagicHeader) {
      return IOSError::Invalid;
   }

   OSLockMutex(&pool->mutex);
   auto index = internal::ipcBufPoolGetMessageIndex(pool, message);

   if (index >= 0) {
      auto messages = reinterpret_cast<uint8_t *>(pool->messages.get());
      auto message = messages + index * pool->messageSize0x18;
      internal::ipcBufPoolFifoPush(&pool->fifo, message);
   } else {
      error = IOSError::Invalid;
   }

   OSUnlockMutex(&pool->mutex);
   return error;
}


namespace internal
{

/**
 * Initialise an IPCBufPoolFIFO structure.
 */
void
ipcBufPoolFifoInit(IPCBufPoolFIFO *fifo,
                   uint32_t numMessages,
                   be_ptr<void> *messages)
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
ipcBufPoolFifoPush(IPCBufPoolFIFO *fifo,
                   void *message)
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
ipcBufPoolFifoPop(IPCBufPoolFIFO *fifo,
                  void **messageOut)
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

   *messageOut = message;
   return IOSError::OK;
}


/**
 * Get the index of a message in an IPCBufPool by it's pointer.
 */
int32_t
ipcBufPoolGetMessageIndex(IPCBufPool *pool,
                          void *message)
{
   if (message < pool->messages) {
      return -1;
   }

   auto offset = reinterpret_cast<uintptr_t>(message) -
                 reinterpret_cast<uintptr_t>(pool->messages.get());

   auto index = offset / pool->messageSize0x18;

   if (index > pool->messageCount) {
      return -1;
   }

   return static_cast<int32_t>(index);
}

} // namespace internal

void
Module::registerIpcBufPoolFunctions()
{
   RegisterKernelFunction(IPCBufPoolCreate);
   RegisterKernelFunction(IPCBufPoolAllocate);
   RegisterKernelFunction(IPCBufPoolFree);
}

} // namespace coreinit
