#pragma once
#include "nn/nn_result.h"
#include "nn/pdm/nn_pdm_cosservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_pdm
{

nn::Result
PDMGetPlayDiaryMaxLength(virt_ptr<uint32_t> outMaxLength);

nn::Result
PDMGetPlayStatsMaxLength(virt_ptr<uint32_t> outMaxLength);

}  // namespace cafe::nn_pdm
