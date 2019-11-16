#include "ios_nim_log.h"
#include "ios_nim_boss_server.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::nim::internal
{

using namespace kernel;

constexpr auto BossNumMessages = 0x64u;
constexpr auto BossThreadStackSize = 0x4000u;
constexpr auto BossThreadPriority = 50u;

class BossServer : public nn::ipc::Server
{
public:
   BossServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticBossServerData
{
   be2_struct<BossServer> server;
   be2_array<Message, BossNumMessages> messageBuffer;
   be2_array<uint8_t, BossThreadStackSize> threadStack;
};

static phys_ptr<StaticBossServerData> sBossServerData = nullptr;

Error
startBossServer()
{
   auto &server = sBossServerData->server;
   auto result = server.initialise("/dev/boss",
                                   phys_addrof(sBossServerData->messageBuffer),
                                   static_cast<uint32_t>(sBossServerData->messageBuffer.size()));
   if (result.failed()) {
      internal::nimLog->error(
         "startBossServer: Server initialisation failed for /dev/boss, result = {}",
         result.code());
      return Error::FailInternal;
   }

   // TODO: Services 0, 1, 2, 3, 4

   result = server.start(phys_addrof(sBossServerData->threadStack) + sBossServerData->threadStack.size(),
                         static_cast<uint32_t>(sBossServerData->threadStack.size()),
                         BossThreadPriority);
   if (result.failed()) {
      internal::nimLog->error(
         "startBossServer: Server start failed for /dev/boss, result = {}",
         result.code());
      return Error::FailInternal;
   }

   return Error::OK;
}

Error
joinBossServer()
{
   sBossServerData->server.join();
   return Error::OK;
}

void
initialiseStaticBossServerData()
{
   sBossServerData = allocProcessStatic<StaticBossServerData>();
}

} // namespace ios::nim::internal
