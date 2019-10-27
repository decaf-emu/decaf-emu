#include "ios_fpd_log.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_act_clientstandardservice.h"
#include "ios_fpd_act_serverstandardservice.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::fpd::internal
{

using namespace kernel;

constexpr auto ActNumMessages = 100u;
constexpr auto ActThreadStackSize = 0x18000u;
constexpr auto ActThreadPriority = 50u;

class ActServer : public nn::ipc::Server
{
public:
   ActServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticActServerData
{
   be2_struct<ActServer> server;
   be2_array<Message, ActNumMessages> messageBuffer;
   be2_array<uint8_t, ActThreadStackSize> threadStack;
};

static phys_ptr<StaticActServerData> sActServerData = nullptr;

Error
startActServer()
{
   initialiseAccounts();

   auto &server = sActServerData->server;
   auto result = server.initialise("/dev/act",
                                   phys_addrof(sActServerData->messageBuffer),
                                   static_cast<uint32_t>(sActServerData->messageBuffer.size()));
   if (result.failed()) {
      internal::fpdLog->error(
         "startActServer: Server initialisation failed for /dev/act, result = {}",
         result.code());
      return Error::FailInternal;
   }

   // TODO: Register services 2, 3, 4, 5, 6
   server.registerService<ActClientStandardService>();
   server.registerService<ActServerStandardService>();

   result = server.start(phys_addrof(sActServerData->threadStack) + sActServerData->threadStack.size(),
                         static_cast<uint32_t>(sActServerData->threadStack.size()),
                         ActThreadPriority);
   if (result.failed()) {
      internal::fpdLog->error(
         "startActServer: Server start failed for /dev/act, result = {}",
         result.code());
      return Error::FailInternal;
   }

   return Error::OK;
}

void
initialiseStaticActServerData()
{
   sActServerData = allocProcessStatic<StaticActServerData>();
}

} // namespace ios::fpd::internal
