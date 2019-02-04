#include "ios_acp_log.h"
#include "ios_acp_pdm_server.h"
#include "ios_acp_pdm_cosservice.h"

#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"

namespace ios::acp::internal
{

using namespace kernel;

constexpr auto PdmNumMessages = 100u;
constexpr auto PdmThreadStackSize = 0x4000u;
constexpr auto PdmThreadPriority = 50u;

class PdmServer : public nn::ipc::Server
{
public:
   PdmServer() :
      nn::ipc::Server(true)
   {
   }
};

struct StaticPdmServerData
{
   be2_struct<PdmServer> server;
   be2_array<Message, PdmNumMessages> messageBuffer;
   be2_array<uint8_t, PdmThreadStackSize> threadStack;
};

static phys_ptr<StaticPdmServerData> sPdmServerData = nullptr;

Error
startPdmServer()
{
   auto &server = sPdmServerData->server;
   auto result = server.initialise("/dev/pdm",
                                   phys_addrof(sPdmServerData->messageBuffer),
                                   static_cast<uint32_t>(sPdmServerData->messageBuffer.size()));
   if (result.failed()) {
      internal::acpLog->error(
         "startPdmServer: Server initialisation failed for /dev/pdm, result = {}",
         result.code);
      return Error::FailInternal;
   }

   // TODO: Register services 1, 2
   server.registerService<PdmCosService>();

   result = server.start(phys_addrof(sPdmServerData->threadStack) + sPdmServerData->threadStack.size(),
                         static_cast<uint32_t>(sPdmServerData->threadStack.size()),
                         PdmThreadPriority);
   if (result.failed()) {
      internal::acpLog->error(
         "startPdmServer: Server start failed for /dev/pdm, result = {}",
         result.code);
      return Error::FailInternal;
   }

   return Error::OK;
}

void
initialiseStaticPdmServerData()
{
   sPdmServerData = allocProcessStatic<StaticPdmServerData>();
}

} // namespace ios::acp::internal
