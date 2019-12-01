#include "ios_fpd_log.h"
#include "ios_fpd_act_accountdata.h"
#include "ios_fpd_act_accountloaderservice.h"
#include "ios_fpd_act_accountmanagerservice.h"
#include "ios_fpd_act_clientstandardservice.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_act_serverstandardservice.h"

#include "ios/auxil/ios_auxil_usr_cfg_ipc.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/nn/ios_nn_ipc_server.h"
#include "ios/fs/ios_fs_fsa_ipc.h"

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

   void intialiseServer() override
   {
      auto error = fs::FSAOpen();
      if (error < Error::OK) {
         internal::fpdLog->error("ActServer::intialiseServer: FSAOpen failed with error = {}", error);
      } else {
         mFsaHandle = static_cast<Handle>(error);
      }

      error = auxil::UCOpen();
      if (error < Error::OK) {
         internal::fpdLog->error("ActServer::intialiseServer: UCOpen failed with error = {}", error);
      } else {
         mUserConfigHandle = static_cast<Handle>(error);
      }

      initialiseAccounts();
   }

   void finaliseServer() override
   {
      if (mFsaHandle >= 0) {
         fs::FSAClose(mFsaHandle);
      }
   }

   Handle getFsaHandle()
   {
      return mFsaHandle;
   }

   Handle getUserConfigHandle()
   {
      return mUserConfigHandle;
   }

private:
   be2_val<Handle> mFsaHandle = -1;
   be2_val<Handle> mUserConfigHandle = -1;
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

   server.registerService<ActClientStandardService>();
   server.registerService<ActServerStandardService>();
   server.registerService<ActAccountManagerService>();
   server.registerService<ActAccountLoaderService>();
   // TODO: Register services 4, 5, 6

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

Handle
getActFsaHandle()
{
   return sActServerData->server.getFsaHandle();
}

Handle
getActUserConfigHandle()
{
   return sActServerData->server.getUserConfigHandle();
}

void
initialiseStaticActServerData()
{
   sActServerData = allocProcessStatic<StaticActServerData>();
}

} // namespace ios::fpd::internal
