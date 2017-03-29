#pragma once
#include "gpu/latte_contextstate.h"
#include "modules/gx2/gx2_displaylist.h"
#include "modules/gx2/gx2_enum.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/cbool.h>
#include <common/structsize.h>
#include <cstdint>

namespace gx2
{

/**
 * \defgroup gx2_contextstate Context State
 * \ingroup gx2
 * @{
 */
#pragma pack(push, 1)

struct GX2ShadowRegisters
{
   be_val<uint32_t> config[0xB00];
   be_val<uint32_t> context[0x400];
   be_val<uint32_t> alu[0x800];
   be_val<uint32_t> loop[0x60];
   PADDING((0x80 - 0x60) * 4);
   be_val<uint32_t> resource[0xD9E];
   PADDING((0xDC0 - 0xD9E) * 4);
   be_val<uint32_t> sampler[0xA2];
   PADDING((0xC0 - 0xA2) * 4);
};
CHECK_OFFSET(GX2ShadowRegisters, 0x0000, config);
CHECK_OFFSET(GX2ShadowRegisters, 0x2C00, context);
CHECK_OFFSET(GX2ShadowRegisters, 0x3C00, alu);
CHECK_OFFSET(GX2ShadowRegisters, 0x5C00, loop);
CHECK_OFFSET(GX2ShadowRegisters, 0x5E00, resource);
CHECK_OFFSET(GX2ShadowRegisters, 0x9500, sampler);
CHECK_SIZE(GX2ShadowRegisters, 0x9800);

// Internal display list is used to create LOAD_ dlist for the shadow state
struct GX2ContextState
{
   static const auto MaxDisplayListSize = 192u;
   GX2ShadowRegisters shadowState;
   be_val<BOOL> profileMode;
   be_val<uint32_t> shadowDisplayListSize;
   // stw 0x9808, value 0 in GX2SetupContextStateEx
   UNKNOWN(0x9e00 - 0x9808);
   be_val<uint32_t> shadowDisplayList[MaxDisplayListSize];
};
CHECK_OFFSET(GX2ContextState, 0x0000, shadowState);
CHECK_OFFSET(GX2ContextState, 0x9800, profileMode);
CHECK_OFFSET(GX2ContextState, 0x9804, shadowDisplayListSize);
CHECK_OFFSET(GX2ContextState, 0x9E00, shadowDisplayList);
CHECK_SIZE(GX2ContextState, 0xA100);

#pragma pack(pop)

void
GX2SetupContextStateEx(GX2ContextState *state,
                       GX2ContextStateFlags flags);

void
GX2GetContextStateDisplayList(GX2ContextState *state,
                              be_ptr<void> *outDisplayList,
                              be_val<uint32_t> *outSize);

void
GX2SetContextState(GX2ContextState *state);

void
GX2SetDefaultState();

namespace internal
{

void
initRegisters();

} // namespace internal

/** @} */

} // namespace gx2
