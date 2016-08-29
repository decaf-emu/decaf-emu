#pragma once
#include "common/types.h"
#include "coreinit_ios.h"

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
