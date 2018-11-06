#pragma once
#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_service.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"

#include <array>
#include <cstdint>

namespace nn::act
{

enum AccountInfoType : uint32_t
{
   Mii = 7,
};

using SlotNo = uint8_t;
using Uuid = std::array<uint8_t, 0x10>;

struct ClientStandardService : ipc::Service<0>
{
   using GetAccountInfo =
      ipc::Command<ClientStandardService, 2>
         ::Parameters<SlotNo, ipc::OutBuffer<void>, AccountInfoType>;

   using FindSlotNoByUuid =
      ipc::Command<ClientStandardService, 23>
         ::Parameters<Uuid, int32_t>
         ::Response<uint8_t>;
};

} // namespace nn::act
