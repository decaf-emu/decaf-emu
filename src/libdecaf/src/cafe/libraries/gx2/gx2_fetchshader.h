#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_shaders Shaders
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2AttribStream;

struct GX2FetchShader
{
   be2_val<GX2FetchShaderType> type;

   struct
   {
      be2_val<latte::SQ_PGM_RESOURCES_FS> sq_pgm_resources_fs;
   } regs;

   be2_val<uint32_t> size;
   be2_virt_ptr<uint8_t> data;
   be2_val<uint32_t> attribCount;
   be2_val<uint32_t> numDivisors;
   be2_val<uint32_t> divisors[2];
};
CHECK_OFFSET(GX2FetchShader, 0x0, type);
CHECK_OFFSET(GX2FetchShader, 0x4, regs.sq_pgm_resources_fs);
CHECK_OFFSET(GX2FetchShader, 0x8, size);
CHECK_OFFSET(GX2FetchShader, 0xC, data);
CHECK_OFFSET(GX2FetchShader, 0x10, attribCount);
CHECK_OFFSET(GX2FetchShader, 0x14, numDivisors);
CHECK_OFFSET(GX2FetchShader, 0x18, divisors);
CHECK_SIZE(GX2FetchShader, 0x20);

#pragma pack(pop)

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType fetchShaderType,
                         GX2TessellationMode tesellationMode);

void
GX2InitFetchShaderEx(virt_ptr<GX2FetchShader> fetchShader,
                     virt_ptr<uint8_t> buffer,
                     uint32_t attribCount,
                     virt_ptr<GX2AttribStream> attribs,
                     GX2FetchShaderType type,
                     GX2TessellationMode tessMode);

/** @} */

} // namespace cafe::gx2
