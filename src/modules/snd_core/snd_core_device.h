#pragma once
#include "modules/snd_core/snd_core_result.h"
#include "types.h"
#include "common/be_val.h"
#include "ppcutils/wfunc_ptr.h"

namespace snd_core
{

namespace AXDeviceType_
{
enum Type : uint32_t
{
   Max = 3,
};
}

using AXDeviceType = AXDeviceType_::Type;

namespace AXDeviceMode_
{
enum Mode : uint32_t
{
};
}

using AXDeviceMode = AXDeviceMode_::Mode;

namespace AXDRCVSMode_
{
enum Mode : uint32_t
{
   Max = 3,
};
}

using AXDRCVSMode = AXDRCVSMode_::Mode;

using AXDeviceFinalMixCallback = wfunc_ptr<void>;
using be_AXDeviceFinalMixCallback = be_wfunc_ptr<void>;
using AXAuxCallback = wfunc_ptr<void>;
using be_AXAuxCallback = wfunc_ptr<void>;

AXResult
AXGetDeviceMode(AXDeviceType type, be_val<AXDeviceMode>* mode);

AXResult
AXGetDeviceFinalMixCallback(AXDeviceType type, be_AXDeviceFinalMixCallback *func);

AXResult
AXRegisterDeviceFinalMixCallback(AXDeviceType type, AXDeviceFinalMixCallback func);

AXResult
AXSetDRCVSMode(AXDRCVSMode mode);

AXResult
AXGetAuxCallback(AXDeviceType type, uint32_t, uint32_t, be_AXAuxCallback *callback, be_ptr<void> *userData);

AXResult
AXRegisterAuxCallback(AXDeviceType type, uint32_t, uint32_t, AXAuxCallback callback, void *userData);

AXResult
AXSetDeviceLinearUpsampler(AXDeviceType type, uint32_t, uint32_t);

AXResult
AXSetDeviceCompressor(AXDeviceType type, uint32_t);

AXResult
AXSetDeviceUpsampleStage(AXDeviceType type, uint32_t);

} // namespace snd_core
