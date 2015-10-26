#pragma once
#include <array_view.h>
#include <string>
#include "gpu/latte.h"
#include "modules/gx2/gx2_shaders.h"

namespace hlsl
{

bool
generateHLSL(const gsl::array_view<GX2AttribStream> &attribs,
             GX2VertexShader *gx2Vertex,
             latte::Shader &vertexShader,
             GX2PixelShader *gx2Pixel,
             latte::Shader &pixelShader,
             std::string &hlsl);

}
