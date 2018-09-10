#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_ipcldriver.h"

#include "cafe/cafe_tinyheap.h"
#include "cafe/kernel/cafe_kernel_ipckdriver.h"
#include "cafe/kernel/cafe_kernel_ipc.h"
#include "cafe/kernel/cafe_kernel_process.h"
#include "ios/mcp/ios_mcp_mcp.h"

#include <array>
#include <cstdint>
#include <common/cbool.h>
#include <libcpu/be2_struct.h>
#include <libcpu/cpu.h>

using namespace ios::mcp;
using namespace cafe::kernel;

namespace cafe::loader::internal
{

constexpr auto HeapSizePerCore = 0x2000u;
constexpr auto HeapAllocAlign = 0x40u;

struct LiLoadReply
{
   be2_val<BOOL> done;
   be2_virt_ptr<void> requestBuffer;
   be2_val<ios::Error> error;
   be2_val<BOOL> pending;
};

struct StaticIopData
{
   be2_val<ios::Handle> mcpHandle;
   be2_struct<LiLoadReply> loadReply;
   alignas(0x40) be2_array<uint8_t, HeapSizePerCore * 3> heapBufs;
};

static virt_ptr<StaticIopData>
sIopData = nullptr;

static virt_ptr<TinyHeap>
iop_percore_getheap()
{
   return virt_cast<TinyHeap *>(
      virt_addrof(sIopData->heapBufs) + HeapSizePerCore * cpu::this_core::id());
}

static void
iop_percore_initheap()
{
   auto heap = iop_percore_getheap();
   auto data = virt_cast<void *>(virt_cast<virt_addr>(heap) + 0x430);
   auto alignedData = align_up(data, HeapAllocAlign);
   auto alignedOffset = virt_cast<virt_addr>(alignedData) - virt_cast<virt_addr>(data);
   TinyHeap_Setup(heap, 0x430, alignedData,
                  HeapSizePerCore - 0x430 - alignedOffset);
}

static virt_ptr<void>
iop_percore_malloc(uint32_t size)
{
   auto heap = iop_percore_getheap();
   size = align_up(size, HeapAllocAlign);

   auto allocPtr = virt_ptr<void> { nullptr };
   TinyHeap_Alloc(heap, size, HeapAllocAlign, &allocPtr);
   return allocPtr;
}

static void
iop_percore_free(virt_ptr<void> buffer)
{
   auto heap = iop_percore_getheap();
   TinyHeap_Free(heap, buffer);
}

void
LiInitIopInterface()
{
   iop_percore_initheap();

   if (sIopData->mcpHandle <= 0) {
      sIopData->mcpHandle = kernel::getMcpHandle();
   }
}

static void
Loader_AsyncCallback(ios::Error error,
                     virt_ptr<void> context)
{
   auto reply = virt_cast<LiLoadReply *>(context);
   if (!reply) {
      return;
   }

   if (reply->requestBuffer) {
      iop_percore_free(reply->requestBuffer);
      reply->requestBuffer = nullptr;
   }

   reply->error = error;
   reply->done = TRUE;
}

static ios::Error
LiPollForCompletion()
{
   virt_ptr<IPCLDriver> driver;
   auto error = IPCLDriver_GetInstance(&driver);
   if (error < ios::Error::OK) {
      return error;
   }

   auto request = ipckDriverLoaderPollCompletion();
   if (!request) {
      return ios::Error::QEmpty;
   }

   return IPCLDriver_ProcessReply(driver, request);
}

int32_t
LiCheckInterrupts()
{
   cpu::this_core::checkInterrupts();
   return 0;
}

void
LiCheckAndHandleInterrupts()
{
   LiCheckInterrupts();
}

ios::Error
LiLoadAsync(std::string_view name,
            virt_ptr<void> outBuffer,
            uint32_t outBufferSize,
            uint32_t pos,
            MCPFileType fileType,
            RamPartitionId rampid)
{
   auto request = virt_cast<MCPRequestLoadFile *>(iop_percore_malloc(sizeof(MCPRequestLoadFile)));
   request->pos = pos;
   request->fileType = fileType;
   request->cafeProcessId = static_cast<uint32_t>(rampid);
   request->name = name;

   sIopData->loadReply.done = FALSE;
   sIopData->loadReply.pending = TRUE;
   sIopData->loadReply.requestBuffer = request;
   sIopData->loadReply.error = ios::Error::InvalidArg;

   auto error = IPCLDriver_IoctlAsync(sIopData->mcpHandle,
                                      MCPCommand::LoadFile,
                                      request, sizeof(MCPRequestLoadFile),
                                      outBuffer, outBufferSize,
                                      &Loader_AsyncCallback,
                                      virt_addrof(sIopData->loadReply));
   if (error < ios::Error::OK) {
      iop_percore_free(request);
   }

   return error;
}

static ios::Error
LiWaitAsyncReply(virt_ptr<LiLoadReply> reply)
{
   while (!reply->done) {
      LiPollForCompletion();
   }

   reply->done = FALSE;
   reply->pending = FALSE;
   return reply->error;
}

static ios::Error
LiWaitAsyncReplyWithInterrupts(virt_ptr<LiLoadReply> reply)
{
   while (!reply->done) {
      LiPollForCompletion();

      if (!reply->done) {
         cpu::this_core::waitNextInterrupt();
      }
   }

   reply->done = FALSE;
   reply->pending = FALSE;
   return reply->error;
}

ios::Error
LiWaitIopComplete(uint32_t *outBytesRead)
{
   auto error = LiWaitAsyncReply(virt_addrof(sIopData->loadReply));
   if (error < 0) {
      *outBytesRead = 0;
   } else {
      *outBytesRead = static_cast<uint32_t>(error);
      error = ios::Error::OK;
   }

   return error;
}

ios::Error
LiWaitIopCompleteWithInterrupts(uint32_t *outBytesRead)
{
   auto error = LiWaitAsyncReplyWithInterrupts(virt_addrof(sIopData->loadReply));
   if (error < 0) {
      *outBytesRead = 0;
   } else {
      *outBytesRead = static_cast<uint32_t>(error);
      error = ios::Error::OK;
   }

   return error;
}

void
initialiseIopStaticData()
{
   sIopData = allocStaticData<StaticIopData>();
}

} // namespace cafe::loader::internal
