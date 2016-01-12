#pragma once
#include "nn_temp_core.h"

TempStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         uint32_t pref,
                         be_val<TempDirID> *idOut);

TempStatus
TEMPShutdownTempDir(TempDirID id);

namespace nn_temp
{

namespace internal
{

std::string
getTempDir(TempDirID id);

} // namespace internal

} // namespace nn_temp
