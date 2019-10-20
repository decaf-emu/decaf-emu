#include "nn_acp.h"
#include "nn_acp_client.h"
#include "nn_acp_saveservice.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/nn/cafe_nn_ipc_client.h"
#include "nn/acp/nn_acp_result.h"
#include "nn/acp/nn_acp_saveservice.h"

using namespace nn::acp;
using namespace nn::ipc;

namespace cafe::nn_acp
{

ACPResult
ACPCreateSaveDir(uint32_t persistentId,
                 ACPDeviceType deviceType)
{
   auto command = ClientCommand<services::SaveService::CreateSaveDir> { internal::getAllocator() };
   command.setParameters(persistentId, deviceType);
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPCreateSaveDir", 771);
}

ACPResult
ACPIsExternalStorageRequired(virt_ptr<int32_t> outRequired)
{
   auto command = ClientCommand<services::SaveService::IsExternalStorageRequired> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      auto required = int32_t{ 0 };
      result = command.readResponse(required);
      if (result.ok()) {
         *outRequired = required;
      }
   }

   return ACPConvertToACPResult(result, "ACPIsExternalStorageRequired", 1464);
}

ACPResult
ACPMountExternalStorage()
{
   auto command = ClientCommand<services::SaveService::MountExternalStorage> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPMountExternalStorage", 1452);
}

ACPResult
ACPMountSaveDir()
{
   auto command = ClientCommand<services::SaveService::MountSaveDir> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPMountSaveDir", 96);
}

ACPResult
ACPRepairSaveMetaDir()
{
   auto command = ClientCommand<services::SaveService::RepairSaveMetaDir> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPRepairSaveMetaDir", 1538);
}

ACPResult
ACPUnmountExternalStorage()
{
   auto command = ClientCommand<services::SaveService::UnmountExternalStorage> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPUnmountExternalStorage", 1458);
}

ACPResult
ACPUnmountSaveDir()
{
   auto command = ClientCommand<services::SaveService::UnmountSaveDir> { internal::getAllocator() };
   auto result = internal::getClient()->sendSyncRequest(command);
   if (result.ok()) {
      result = command.readResponse();
   }

   return ACPConvertToACPResult(result, "ACPUnmountSaveDir", 105);
}

void
Library::registerSaveServiceSymbols()
{
   RegisterFunctionExport(ACPCreateSaveDir);
   RegisterFunctionExport(ACPIsExternalStorageRequired);
   RegisterFunctionExport(ACPMountExternalStorage);
   RegisterFunctionExport(ACPMountSaveDir);
   RegisterFunctionExport(ACPRepairSaveMetaDir);
   RegisterFunctionExport(ACPUnmountExternalStorage);
   RegisterFunctionExport(ACPUnmountSaveDir);

   RegisterFunctionExportName("CreateSaveDir__Q2_2nn3acpFUi13ACPDeviceType",
                              ACPCreateSaveDir);
   RegisterFunctionExportName("RepairSaveMetaDir__Q2_2nn3acpFv",
                              ACPRepairSaveMetaDir);
}

}  // namespace cafe::nn_acp
