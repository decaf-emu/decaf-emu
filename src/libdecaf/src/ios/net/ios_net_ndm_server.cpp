#include "ios_net_log.h"
#include "ios_net_ndm_server.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::net::internal
{

using namespace kernel;

constexpr auto NdmNumMessages = 0x64u;
constexpr auto NdmThreadStackSize = 0x2000u;
constexpr auto NdmThreadPriority = 45u;

class NdmServer : public nn::ipc::Server
{
public:
   NdmServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticNdmServerData
{
   be2_struct<NdmServer> server;
   be2_array<Message, NdmNumMessages> messageBuffer;
   be2_array<uint8_t, NdmThreadStackSize> threadStack;
};

static phys_ptr<StaticNdmServerData> sNdmServerData = nullptr;

Error
startNdmServer()
{
   auto &server = sNdmServerData->server;
   auto result = server.initialise("/dev/ndm",
                                   phys_addrof(sNdmServerData->messageBuffer),
                                   static_cast<uint32_t>(sNdmServerData->messageBuffer.size()));
   if (result.failed()) {
      netLog->error(
         "startNdmServer: Server initialisation failed for /dev/ndm, result = {}",
         result.code());
      return Error::FailInternal;
   }

   // TODO: Register service 0, 1, 2

   result = server.start(phys_addrof(sNdmServerData->threadStack) + sNdmServerData->threadStack.size(),
                         static_cast<uint32_t>(sNdmServerData->threadStack.size()),
                         NdmThreadPriority);
   if (result.failed()) {
      netLog->error(
         "startNdmServer: Server start failed for /dev/ndm, result = {}",
         result.code());
      return Error::FailInternal;
   }

   return Error::OK;
}

Error
joinNdmServer()
{
   sNdmServerData->server.join();
   return Error::OK;
}

void
initialiseStaticNdmServerData()
{
   sNdmServerData = allocProcessStatic<StaticNdmServerData>();
}

} // namespace ios::net::internal
