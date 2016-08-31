#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "snd_core_enum.h"

namespace snd_core
{

struct AXFXChorus;
struct AXFXDelay;
struct AXFXReverbHi;
struct AXFXReverbStd;

int32_t
AXFXChorusExpGetMemSize(AXFXChorus *chorus);

int32_t
AXFXDelayExpGetMemSize(AXFXDelay *chorus);

int32_t
AXFXReverbHiExpGetMemSize(AXFXReverbHi *chorus);

int32_t
AXFXReverbStdExpGetMemSize(AXFXReverbStd *chorus);

} // namespace snd_core
