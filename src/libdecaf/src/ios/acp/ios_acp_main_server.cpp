#include "ios_acp_log.h"
#include "ios_acp_main_server.h"
#include "ios_acp_nn_miscservice.h"
#include "ios_acp_nn_saveservice.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::acp::internal
{

using namespace kernel;

constexpr auto AcpMainNumMessages = 100u;
constexpr auto AcpMainThreadStackSize = 0x10000u;
constexpr auto AcpMainThreadPriority = 50u;

class AcpMainServer : public nn::ipc::Server
{
public:
   AcpMainServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticAcpMainServerData
{
   be2_struct<AcpMainServer> server;
   be2_array<Message, AcpMainNumMessages> messageBuffer;
   be2_array<uint8_t, AcpMainThreadStackSize> threadStack;
};

static phys_ptr<StaticAcpMainServerData> sMainServerData = nullptr;

Error
startMainServer()
{
   auto &server = sMainServerData->server;
   auto result = server.initialise("/dev/acp_main",
                                   phys_addrof(sMainServerData->messageBuffer),
                                   static_cast<uint32_t>(sMainServerData->messageBuffer.size()));
   if (result.failed()) {
      internal::acpLog->error(
         "startMainServer: Server initialisation failed for /dev/acp_main, result = {}",
         result.code);
      return Error::FailInternal;
   }

   // TODO: Services 1, 3, 4, 6, 301, 302, 303, 304
   server.registerService<MiscService>();
   server.registerService<SaveService>();

   result = server.start(phys_addrof(sMainServerData->threadStack) + sMainServerData->threadStack.size(),
                         static_cast<uint32_t>(sMainServerData->threadStack.size()),
                         AcpMainThreadPriority);
   if (result.failed()) {
      internal::acpLog->error(
         "startMainServer: Server start failed for /dev/acp_main, result = {}",
         result.code);
      return Error::FailInternal;
   }

   return Error::OK;
}

void
initialiseStaticMainServerData()
{
   sMainServerData = allocProcessStatic<StaticAcpMainServerData>();
}

} // namespace ios::acp::internal
