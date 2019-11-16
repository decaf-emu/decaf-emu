#include "ios_nim_log.h"
#include "ios_nim_nim_server.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::nim::internal
{

using namespace kernel;

constexpr auto NimNumMessages = 0x64u;
constexpr auto NimThreadStackSize = 0x8000u;
constexpr auto NimThreadPriority = 50u;

class NimServer : public nn::ipc::Server
{
public:
   NimServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticNimServerData
{
   be2_struct<NimServer> server;
   be2_array<Message, NimNumMessages> messageBuffer;
   be2_array<uint8_t, NimThreadStackSize> threadStack;
};

static phys_ptr<StaticNimServerData> sNimServerData = nullptr;

Error
startNimServer()
{
   auto &server = sNimServerData->server;
   auto result = server.initialise("/dev/nim",
                                   phys_addrof(sNimServerData->messageBuffer),
                                   static_cast<uint32_t>(sNimServerData->messageBuffer.size()));
   if (result.failed()) {
      internal::nimLog->error(
         "startMainServer: Server initialisation failed for /dev/nim, result = {}",
         result.code());
      return Error::FailInternal;
   }

   // TODO: Services 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18

   result = server.start(phys_addrof(sNimServerData->threadStack) + sNimServerData->threadStack.size(),
                         static_cast<uint32_t>(sNimServerData->threadStack.size()),
                         NimThreadPriority);
   if (result.failed()) {
      internal::nimLog->error(
         "startMainServer: Server start failed for /dev/nim, result = {}",
         result.code());
      return Error::FailInternal;
   }

   return Error::OK;
}

Error
joinNimServer()
{
   sNimServerData->server.join();
   return Error::OK;
}

void
initialiseStaticNimServerData()
{
   sNimServerData = allocProcessStatic<StaticNimServerData>();
}

} // namespace ios::nim::internal
