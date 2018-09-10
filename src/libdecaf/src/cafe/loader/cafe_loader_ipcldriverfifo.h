#pragma once
#include "ios/ios_enum.h"
#include "ios/ios_ipc.h"
#include "cafe/kernel/cafe_kernel_ipckdriver.h"

#include <cstdint>
#include <common/cbool.h>
#include <functional>
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

#pragma pack(push, 1)

struct IPCLDriverRequest;

/**
 * FIFO queue for IPCLDriverRequests.
 *
 * Functions similar to a ring buffer.
 */
template<size_t MaxSize>
struct IPCLDriverFIFO
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
   be2_array<virt_ptr<IPCLDriverRequest>, MaxSize> requests;
};
CHECK_OFFSET(IPCLDriverFIFO<4>, 0x00, pushIndex);
CHECK_OFFSET(IPCLDriverFIFO<4>, 0x04, popIndex);
CHECK_OFFSET(IPCLDriverFIFO<4>, 0x08, count);
CHECK_OFFSET(IPCLDriverFIFO<4>, 0x0C, maxCount);
CHECK_OFFSET(IPCLDriverFIFO<4>, 0x10, requests);
CHECK_SIZE(IPCLDriverFIFO<4>, 0x20);

#pragma pack(pop)


/**
 * Initialise a IPCLDriverFIFO structure.
 */
template<size_t MaxSize>
inline void
IPCLDriver_FIFOInit(virt_ptr<IPCLDriverFIFO<MaxSize>> fifo)
{
   fifo->pushIndex = 0;
   fifo->popIndex = -1;
   fifo->count = 0;
   fifo->requests.fill(nullptr);
}


/**
 * Push a request into an IPCLDriverFIFO structure
 *
 * \retval ios::Error::OK
 * Success.
 *
 * \retval ios::Error::QFull
 * There was no free space in the queue to push the request.
 */
template<size_t MaxSize>
inline ios::Error
IPCLDriver_FIFOPush(virt_ptr<IPCLDriverFIFO<MaxSize>> fifo,
                    virt_ptr<IPCLDriverRequest> request)
{
   if (fifo->pushIndex == fifo->popIndex) {
      return ios::Error::QFull;
   }

   fifo->requests[fifo->pushIndex] = request;

   if (fifo->popIndex == -1) {
      fifo->popIndex = fifo->pushIndex;
   }

   fifo->count += 1;
   fifo->pushIndex = static_cast<int32_t>((fifo->pushIndex + 1) % IPCLBufferCount);

   if (fifo->count > fifo->maxCount) {
      fifo->maxCount = fifo->count;
   }

   return ios::Error::OK;
}


/**
 * Pop a request into an IPCLDriverFIFO structure.
 *
 * \retval ios::Error::OK
 * Success.
 *
 * \retval ios::Error::QEmpty
 * There was no requests to pop from the queue.
 */
template<size_t MaxSize>
inline ios::Error
IPCLDriver_FIFOPop(virt_ptr<IPCLDriverFIFO<MaxSize>> fifo,
                   virt_ptr<IPCLDriverRequest> *outRequest)
{
   if (fifo->popIndex == -1) {
      return ios::Error::QEmpty;
   }

   auto request = fifo->requests[fifo->popIndex];
   fifo->count -= 1;

   if (fifo->count == 0) {
      fifo->popIndex = -1;
   } else {
      fifo->popIndex = static_cast<int32_t>((fifo->popIndex + 1) % IPCLBufferCount);
   }

   *outRequest = request;
   return ios::Error::OK;
}


/**
 * Peek the next request which would be popped from a IPCLDriverFIFO structure.
 *
 * \retval ios::Error::OK
 * Success.
 *
 * \retval ios::Error::QEmpty
 * There was no requests to pop from the queue.
 */
template<size_t MaxSize>
ios::Error
IPCLDriver_PeekFIFO(virt_ptr<IPCLDriverFIFO<MaxSize>> fifo,
                    virt_ptr<virt_ptr<IPCLDriverRequest>> outRequest)
{
   if (fifo->popIndex == -1) {
      return ios::Error::QEmpty;
   }

   *outRequest = fifo->requests[fifo->popIndex];
   return ios::Error::OK;
}

} // namespace cafe::loader::internal
