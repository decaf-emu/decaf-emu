#pragma once
#include "ios/ios_error.h"

#include <libcpu/be2_struct.h>

namespace cafe::kernel::internal
{

struct IPCKDriverRequestBlock;

/**
 * FIFO queue for IPCKDriverRequestBlocks.
 *
 * Functions similar to a ring buffer.
 */
template<std::size_t Size>
struct IPCKDriverFIFO
{
   //! The current item index to push to
   be2_val<int32_t> pushIndex;

   //! The current item index to pop from
   be2_val<int32_t> popIndex;

   //! The number of items in the queue
   be2_val<int32_t> count;

   //! Tracks the highest amount of items there has been in the queue
   be2_val<int32_t> maxCount;

   //! Items in the queue
   be2_array<virt_ptr<IPCKDriverRequestBlock>, Size> requestBlocks;
};
CHECK_OFFSET(IPCKDriverFIFO<1>, 0x00, pushIndex);
CHECK_OFFSET(IPCKDriverFIFO<1>, 0x04, popIndex);
CHECK_OFFSET(IPCKDriverFIFO<1>, 0x08, count);
CHECK_OFFSET(IPCKDriverFIFO<1>, 0x0C, maxCount);
CHECK_OFFSET(IPCKDriverFIFO<1>, 0x10, requestBlocks);
CHECK_SIZE(IPCKDriverFIFO<1>, 0x14);

/**
 * Initialises an IPCKDriverFIFO structure.
 */
template<std::size_t Size>
static void
IPCKDriver_FIFOInit(virt_ptr<IPCKDriverFIFO<Size>> fifo)
{
   fifo->pushIndex = 0;
   fifo->popIndex = -1;
   fifo->count = 0;
   fifo->requestBlocks.fill(nullptr);
}


/**
 * Push a request into an IPCKDriverFIFO structure.
 *
 * \retval ios::Error::OK
 * Success.
 *
 * \retval ios::Error::QFull
 * There was no free space in the queue to push the request.
 */
template<std::size_t Size>
static ios::Error
IPCKDriver_FIFOPush(virt_ptr<IPCKDriverFIFO<Size>> fifo,
                    virt_ptr<IPCKDriverRequestBlock> requestBlock)
{
   if (fifo->pushIndex == fifo->popIndex) {
      return ios::Error::QFull;
   }

   fifo->requestBlocks[fifo->pushIndex] = requestBlock;

   if (fifo->popIndex == -1) {
      fifo->popIndex = fifo->pushIndex;
   }

   fifo->count += 1;
   fifo->pushIndex = static_cast<int32_t>((fifo->pushIndex + 1) % Size);

   if (fifo->count > fifo->maxCount) {
      fifo->maxCount = fifo->count;
   }

   return ios::Error::OK;
}


/**
 * Pop a request from an IPCKDriverFIFO structure.
 *
 * \retval ios::Error::OK
 * Success.
 *
 * \retval ios::Error::QEmpty
 * There was no requests to pop from the queue.
 */
template<std::size_t Size>
static ios::Error
IPCKDriver_FIFOPop(virt_ptr<IPCKDriverFIFO<Size>> fifo,
                   virt_ptr<IPCKDriverRequestBlock> *outRequestBlock)
{
   if (fifo->popIndex == -1) {
      return ios::Error::QEmpty;
   }

   auto requestBlock = fifo->requestBlocks[fifo->popIndex];
   fifo->count -= 1;

   if (fifo->count == 0) {
      fifo->popIndex = -1;
   } else {
      fifo->popIndex = static_cast<int32_t>((fifo->popIndex + 1) % Size);
   }

   *outRequestBlock = requestBlock;
   return ios::Error::OK;
}

} // namespace cafe::kernel::internal
