#pragma once
#include "gx2_displaylist.h"
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_contextstate Context State
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2ShadowRegisters
{
   be2_array<uint32_t, 0xB00> config;
   be2_array<uint32_t, 0x400> context;
   be2_array<uint32_t, 0x800> alu;
   be2_array<uint32_t, 0x60> loop;
   PADDING((0x80 - 0x60) * 4);
   be2_array<uint32_t, 0xD9E> resource;
   PADDING((0xDC0 - 0xD9E) * 4);
   be2_array<uint32_t, 0xA2> sampler;
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
   be2_struct<GX2ShadowRegisters> shadowState;
   be2_val<BOOL> profileMode;
   be2_val<uint32_t> shadowDisplayListSize;
   // stw 0x9808, value 0 in GX2SetupContextStateEx
   UNKNOWN(0x9e00 - 0x9808);
   be2_array<uint32_t, MaxDisplayListSize> shadowDisplayList;
};
CHECK_OFFSET(GX2ContextState, 0x0000, shadowState);
CHECK_OFFSET(GX2ContextState, 0x9800, profileMode);
CHECK_OFFSET(GX2ContextState, 0x9804, shadowDisplayListSize);
CHECK_OFFSET(GX2ContextState, 0x9E00, shadowDisplayList);
CHECK_SIZE(GX2ContextState, 0xA100);

#pragma pack(pop)

void
GX2SetupContextStateEx(virt_ptr<GX2ContextState> state,
                       GX2ContextStateFlags flags);

void
GX2GetContextStateDisplayList(virt_ptr<GX2ContextState> state,
                              virt_ptr<virt_ptr<void>> outDisplayList,
                              virt_ptr<uint32_t> outSize);

void
GX2SetContextState(virt_ptr<GX2ContextState> state);

void
GX2SetDefaultState();

namespace internal
{

void
initialiseRegisters();

} // namespace internal

/** @} */

} // namespace cafe::gx2
