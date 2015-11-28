#pragma once
#include "modules/snd_core/snd_core_result.h"
#include "types.h"
#include "utils/be_val.h"
#include "utils/wfunc_ptr.h"

namespace AXDeviceType
{
enum Type : uint32_t
{
   Max = 3,
};
}

namespace AXDeviceMode
{
enum Mode : uint32_t
{
};
}

namespace AXDRCVSMode
{
enum Mode : uint32_t
{
   Max = 3,
};
}

using AXDeviceFinalMixCallback = wfunc_ptr<void>;
using be_AXDeviceFinalMixCallback = be_wfunc_ptr<void>;
using AXAuxCallback = wfunc_ptr<void>;
using be_AXAuxCallback = wfunc_ptr<void>;

AXResult::Result
AXGetDeviceMode(AXDeviceType::Type type, be_val<AXDeviceMode::Mode>* mode);

AXResult::Result
AXGetDeviceFinalMixCallback(AXDeviceType::Type type, be_AXDeviceFinalMixCallback *func);

AXResult::Result
AXRegisterDeviceFinalMixCallback(AXDeviceType::Type type, AXDeviceFinalMixCallback func);

AXResult::Result
AXSetDRCVSMode(AXDRCVSMode::Mode mode);

AXResult::Result
AXGetAuxCallback(AXDeviceType::Type type, uint32_t, uint32_t, be_AXAuxCallback *callback, be_ptr<void> *userData);

AXResult::Result
AXRegisterAuxCallback(AXDeviceType::Type type, uint32_t, uint32_t, AXAuxCallback callback, void *userData);
