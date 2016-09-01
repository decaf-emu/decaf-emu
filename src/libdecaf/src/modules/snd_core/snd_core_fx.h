#pragma once
#include "common/types.h"
#include "common/be_val.h"
#include "snd_core_enum.h"

namespace snd_core
{

struct AXFXBuffers
{
   int32_t *left;
   int32_t *right;
   int32_t *surround;
};

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

void
AXFXChorusExpCallback(AXFXBuffers *buffers, AXFXChorus *data);

void
AXFXDelayExpCallback(AXFXBuffers *buffers, AXFXDelay *data);

void
AXFXReverbHiExpCallback(AXFXBuffers *buffers, AXFXReverbHi *data);

void
AXFXReverbStdExpCallback(AXFXBuffers *buffers, AXFXReverbStd *data);

} // namespace snd_core
