#pragma once
#include "coreinit_ios.h"

#include <cstdint>
#include <common/be_val.h>
#include <common/cbool.h>

namespace coreinit
{

/**
 * \defgroup coreinit_im IM
 * \ingroup coreinit
 * @{
 */

IMError
IMDisableAPD();

IMError
IMDisableDim();

IMError
IMEnableAPD();

IMError
IMEnableDim();

BOOL
IMIsAPDEnabled();

BOOL
IMIsAPDEnabledBySysSettings();

BOOL
IMIsDimEnabled();

IMError
IMGetTimeBeforeAPD(be_val<uint32_t> *seconds);

IMError
IMGetTimeBeforeDimming(be_val<uint32_t> *seconds);

/** @} */

} // namespace coreinit
