#pragma once
#include "dx12_state.h"

struct FetchShaderInfo {
   GX2FetchShaderType::Type type;
   GX2TessellationMode::Mode tessMode;
   GX2AttribStream attribs[1];
};
