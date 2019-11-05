#pragma once
#include "nn_act_enum.h"
#include "nn_act_types.h"

#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

#include <array>
#include <cstdint>

namespace nn::act::services
{

struct ClientStandardService : ipc::Service<0>
{
   using GetCommonInfo =
      ipc::Command<ClientStandardService, 1>
         ::Parameters<ipc::OutBuffer<void>, InfoType>;

   using GetAccountInfo =
      ipc::Command<ClientStandardService, 2>
         ::Parameters<SlotNo, ipc::OutBuffer<void>, InfoType>;

   using GetTransferableId =
      ipc::Command<ClientStandardService, 4>
         ::Parameters<SlotNo, uint32_t>
         ::Response<TransferrableId>;

   using GetMiiImage =
      ipc::Command<ClientStandardService, 6>
         ::Parameters<SlotNo, ipc::OutBuffer<void>, MiiImageType>
         ::Response<uint32_t>;

   using GetUuid =
      ipc::Command<ClientStandardService, 22>
         ::Parameters<SlotNo, ipc::OutBuffer<Uuid>, int32_t>;

   using FindSlotNoByUuid =
      ipc::Command<ClientStandardService, 23>
         ::Parameters<Uuid, int32_t>
         ::Response<uint8_t>;
};

} // namespace nn::act::services
