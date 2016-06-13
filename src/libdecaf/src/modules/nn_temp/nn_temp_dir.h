#pragma once
#include "nn_temp_core.h"

namespace nn
{

namespace temp
{

TempStatus
TEMPCreateAndInitTempDir(uint32_t maxSize,
                         uint32_t pref,
                         be_val<TempDirID> *idOut);

TempStatus
TEMPShutdownTempDir(TempDirID id);

namespace internal
{

std::string
getTempDir(TempDirID id);

} // namespace internal

} // namespace temp

} // namespace nn
