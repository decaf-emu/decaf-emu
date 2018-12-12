#pragma once
#include "nn_acp_types.h"

#include "nn/ipc/nn_ipc_command.h"
#include "nn/ipc/nn_ipc_managedbuffer.h"
#include "nn/ipc/nn_ipc_service.h"

#include <cstdint>

namespace nn::acp::services
{

struct MiscService : ipc::Service<0>
{
   using GetNetworkTime =
      ipc::Command<MiscService, 101>
         ::Parameters<>
         ::Response<int64_t, uint32_t>;

   using GetTitleIdOfMainApplication =
      ipc::Command<MiscService, 201>
      ::Parameters<>
      ::Response<ACPTitleId>;

   using GetTitleMetaXml =
      ipc::Command<MiscService, 205>
         ::Parameters<ipc::OutBuffer<ACPMetaXml>, ACPTitleId>;
};

} // namespace nn::acp::services
