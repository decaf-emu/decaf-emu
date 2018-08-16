#pragma once
#include "latte_registers.h"

#include <common/bitfield.h>
#include <libcpu/be2_struct.h>
#include <cstdint>

#pragma pack(push, 1)

namespace latte
{

BITFIELD(CONTEXT_CONTROL_ENABLE, uint32_t)
   BITFIELD_ENTRY(0, 1, bool, ENABLE_CONFIG_REG);
   BITFIELD_ENTRY(1, 1, bool, ENABLE_CONTEXT_REG);
   BITFIELD_ENTRY(2, 1, bool, ENABLE_ALU_CONST);
   BITFIELD_ENTRY(3, 1, bool, ENABLE_BOOL_CONST);
   BITFIELD_ENTRY(4, 1, bool, ENABLE_LOOP_CONST);
   BITFIELD_ENTRY(5, 1, bool, ENABLE_RESOURCE);
   BITFIELD_ENTRY(6, 1, bool, ENABLE_SAMPLER);
   BITFIELD_ENTRY(7, 1, bool, ENABLE_CTL_CONST);
   BITFIELD_ENTRY(31, 1, bool, ENABLE_ORDINAL);
BITFIELD_END

struct ShadowState
{
   CONTEXT_CONTROL_ENABLE LOAD_CONTROL = CONTEXT_CONTROL_ENABLE::get(0);
   CONTEXT_CONTROL_ENABLE SHADOW_ENABLE = CONTEXT_CONTROL_ENABLE::get(0);
   virt_ptr<uint32_t> CONFIG_REG_BASE = nullptr;
   virt_ptr<uint32_t> CONTEXT_REG_BASE = nullptr;
   virt_ptr<uint32_t> ALU_CONST_BASE = nullptr;
   virt_ptr<uint32_t> BOOL_CONST_BASE = nullptr;
   virt_ptr<uint32_t> LOOP_CONST_BASE = nullptr;
   virt_ptr<uint32_t> RESOURCE_CONST_BASE = nullptr;
   virt_ptr<uint32_t> SAMPLER_CONST_BASE = nullptr;
   virt_ptr<uint32_t> CTL_CONST_BASE = nullptr;
};

} // namespace latte

#pragma pack(pop)
