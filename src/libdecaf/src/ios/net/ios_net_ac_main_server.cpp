#include "ios_net_log.h"
#include "ios_net_ac_main_server.h"
#include "ios_net_ac_service.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::net::internal
{

using namespace kernel;

constexpr auto AcMainNumMessages = 0x64u;
constexpr auto AcMainThreadStackSize = 0x2000u;
constexpr auto AcMainThreadPriority = 50u;

class AcMainServer : public nn::ipc::Server
{
public:
   AcMainServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticAcMainServerData
{
   be2_struct<AcMainServer> server;
   be2_array<Message, AcMainNumMessages> messageBuffer;
   be2_array<uint8_t, AcMainThreadStackSize> threadStack;
};

static phys_ptr<StaticAcMainServerData> sAcMainServerData = nullptr;

Error
startAcMainServer()
{
   auto &server = sAcMainServerData->server;
   auto result = server.initialise("/dev/ac_main",
                                   phys_addrof(sAcMainServerData->messageBuffer),
                                   static_cast<uint32_t>(sAcMainServerData->messageBuffer.size()));
   if (result.failed()) {
      netLog->error(
         "startAcMainServer: Server initialisation failed for /dev/ac_main, result = {}",
         result.code());
      return Error::FailInternal;
   }

   server.registerService<AcService>();

   result = server.start(phys_addrof(sAcMainServerData->threadStack) + sAcMainServerData->threadStack.size(),
                         static_cast<uint32_t>(sAcMainServerData->threadStack.size()),
                         AcMainThreadPriority);
   if (result.failed()) {
      netLog->error(
         "startAcMainServer: Server start failed for /dev/ac_main, result = {}",
         result.code());
      return Error::FailInternal;
   }

   return Error::OK;
}

Error
joinAcMainServer()
{
   sAcMainServerData->server.join();
   return Error::OK;
}

void
initialiseStaticAcMainServerData()
{
   sAcMainServerData = allocProcessStatic<StaticAcMainServerData>();
}

} // namespace ios::net::internal
