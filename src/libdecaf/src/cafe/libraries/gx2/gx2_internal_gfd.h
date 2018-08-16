#pragma once
#include "gx2_shaders.h"
#include "gx2_surface.h"
#include "gx2_texture.h"

#include <libgfd/gfd.h>

namespace cafe::gx2::internal
{

void
gfdToGX2Surface(const gfd::GFDSurface &src,
                GX2Surface *dst);
void
gx2ToGFDSurface(const GX2Surface *src,
                gfd::GFDSurface &dst);

void
gfdToGX2Texture(const gfd::GFDTexture &src,
                GX2Texture *dst);

void
gx2ToGFDTexture(const GX2Texture *src,
                gfd::GFDTexture &dst);

void
gx2ToGFDVertexShader(const GX2VertexShader *src,
                     gfd::GFDVertexShader &dst);

void
gx2ToGFDPixelShader(const GX2PixelShader *src,
                    gfd::GFDPixelShader &dst);

void
gx2ToGFDGeometryShader(const GX2GeometryShader *src,
                       gfd::GFDGeometryShader &dst);

} // namespace cafe::gx2::internal
