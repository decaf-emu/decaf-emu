#pragma once
#include "coreinit_ios.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/cbool.h>

namespace coreinit
{

IOError
IMDisableAPD();

IOError
IMDisableDim();

IOError
IMEnableAPD();

IOError
IMEnableDim();

BOOL
IMIsAPDEnabled();

BOOL
IMIsAPDEnabledBySysSettings();

BOOL
IMIsDimEnabled();

IOError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds);

IOError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds);

} // namespace coreinit
