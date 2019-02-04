#pragma once
#include "nn/ipc/nn_ipc_service.h"

#include <cstdint>

namespace nn::pdm::services
{

struct CosService : ipc::Service<0>
{
   using GetPlayDiaryMaxLength =
      ipc::Command<CosService, 0>
      ::Parameters<>
      ::Response<uint32_t>;

   using GetPlayStatsMaxLength =
      ipc::Command<CosService, 256>
      ::Parameters<>
      ::Response<uint32_t>;
};

} // namespace nn::pdm::services
