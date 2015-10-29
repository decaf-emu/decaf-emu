#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_debug.h"
#include "modules/gx2/gx2_texture.h"
#include "dx12_state.h"
#include "dx12_texture.h"

void
GX2InitTextureRegs(GX2Texture *texture)
{
   // TODO: GX2InitTextureRegs
}

void
_GX2SetPixelTexture(GX2Texture *texture,
                   uint32_t unit)
{
   gLog->debug("_GX2SetPixelTexture({}, {})", memory_untranslate(texture), unit);

   GX2DumpTexture(texture);
   DXTextureData *textureData = dx::getTexture(texture);
   textureData->upload();

   gDX.activeTextures[unit] = textureData;
}

void
GX2SetPixelTexture(GX2Texture *texture,
   uint32_t unit)
{
   DX_DLCALL(_GX2SetPixelTexture, texture, unit);
}

#endif
