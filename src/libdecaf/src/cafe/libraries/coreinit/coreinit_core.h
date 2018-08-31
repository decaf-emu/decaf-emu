#pragma once
#include <cstdint>
#include <common/cbool.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_core Core Identification
 * \ingroup coreinit
 * @{
 */

static constexpr auto MainCoreId = 1u;
static constexpr auto CoreCount = 3u;

constexpr uint32_t
OSGetCoreCount()
{
   return CoreCount;
}

constexpr uint32_t
OSGetMainCoreId()
{
   return MainCoreId;
}

uint32_t
OSGetCoreId();

BOOL
OSIsMainCore();

/** @} */

} // namespace cafe::coreinit
