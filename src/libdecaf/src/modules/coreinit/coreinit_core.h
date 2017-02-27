#pragma once
#include <cstdint>
#include <common/cbool.h>

namespace coreinit
{

/**
 * \defgroup coreinit_core Core Identification
 * \ingroup coreinit
 * @{
 */

static const uint32_t CoreCount = 3;

uint32_t
OSGetCoreCount();

uint32_t
OSGetCoreId();

uint32_t
OSGetMainCoreId();

BOOL
OSIsMainCore();

/** @} */

} // namespace coreinit
