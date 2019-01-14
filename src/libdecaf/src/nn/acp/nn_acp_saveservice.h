#pragma once
#include "nn_acp_enum.h"
#include "nn_acp_types.h"

#include "nn/ipc/nn_ipc_service.h"

#include <cstdint>

namespace nn::acp::services
{

struct SaveService : ipc::Service<2>
{
   using MountSaveDir =
      ipc::Command<SaveService, 101>
      ::Parameters<>;

   using UnmountSaveDir =
      ipc::Command<SaveService, 102>
      ::Parameters<>;

   using CreateSaveDir =
      ipc::Command<SaveService, 103>
      ::Parameters<uint32_t, ACPDeviceType>;

   using RepairSaveMetaDir =
      ipc::Command<SaveService, 125>
      ::Parameters<>;

   using MountExternalStorage =
      ipc::Command<SaveService, 150>
      ::Parameters<>;

   using UnmountExternalStorage =
      ipc::Command<SaveService, 151>
      ::Parameters<>;

   using IsExternalStorageRequired =
      ipc::Command<SaveService, 152>
      ::Parameters<>
      ::Response<int32_t>;
};

} // namespace nn::acp::services
