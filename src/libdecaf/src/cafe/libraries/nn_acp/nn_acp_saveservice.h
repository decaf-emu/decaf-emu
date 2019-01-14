#pragma once
#include "nn/nn_result.h"
#include "nn/acp/nn_acp_saveservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

using ACPDeviceType = nn::acp::ACPDeviceType;

nn::Result
ACPCreateSaveDir(uint32_t persistentId,
                 ACPDeviceType deviceType);

nn::Result
ACPIsExternalStorageRequired(virt_ptr<int32_t> outRequired);

nn::Result
ACPMountExternalStorage();

nn::Result
ACPMountSaveDir();

nn::Result
ACPRepairSaveMetaDir();

nn::Result
ACPUnmountExternalStorage();

nn::Result
ACPUnmountSaveDir();

}  // namespace cafe::nn_acp
