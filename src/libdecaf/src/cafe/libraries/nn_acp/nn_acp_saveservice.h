#pragma once
#include "nn_acp_acpresult.h"
#include "nn/acp/nn_acp_saveservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_acp
{

using ACPDeviceType = nn::acp::ACPDeviceType;

ACPResult
ACPCreateSaveDir(uint32_t persistentId,
                 ACPDeviceType deviceType);

ACPResult
ACPIsExternalStorageRequired(virt_ptr<int32_t> outRequired);

ACPResult
ACPMountExternalStorage();

ACPResult
ACPMountSaveDir();

ACPResult
ACPRepairSaveMetaDir();

ACPResult
ACPUnmountExternalStorage();

ACPResult
ACPUnmountSaveDir();

}  // namespace cafe::nn_acp
