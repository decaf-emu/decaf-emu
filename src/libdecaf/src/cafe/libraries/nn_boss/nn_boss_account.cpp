#include "nn_boss.h"
#include "nn_boss_account.h"
#include "nn_boss_client.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/boss/nn_boss_result.h"
#include "nn/boss/nn_boss_privileged_service.h"

using namespace nn::boss;
using namespace nn::boss::services;
using namespace nn::ipc;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> Account::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> Account::TypeDescriptor = nullptr;

virt_ptr<Account>
Account_Constructor(virt_ptr<Account> self,
                    uint32_t a1)
{
   if (!self) {
      self = virt_cast<Account *>(ghs::malloc(sizeof(Account)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = Account::VirtualTable;
   self->unk0x00 = a1;
   return self;
}

void
Account_Destructor(virt_ptr<Account> self,
                   ghs::DestructorFlags flags)
{
   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
Account_AddAccount(PersistentId persistentId)
{
   if (!IsInitialized()) {
      return ResultLibraryNotInitialiased;
   }

   auto command = ClientCommand<PrivilegedService::AddAccount> { internal::getAllocator() };
   command.setParameters(persistentId);

   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return result;
}

void
Library::registerAccountSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss7AccountFUi",
                              Account_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss7AccountFv",
                              Account_Destructor);

   RegisterFunctionExportName("AddAccount__Q3_2nn4boss7AccountSFUi",
                              Account_AddAccount);

   RegisterTypeInfo(
      Account,
      "nn::boss::Account",
      {
         "__dt__Q3_2nn4boss7AccountFv",
      },
      {});
}

}  // namespace namespace cafe::nn_boss
