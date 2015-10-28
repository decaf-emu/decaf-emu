#pragma once
#include "dx12_state.h"

struct FetchShaderInfo
{
   GX2FetchShaderType::Value type;
   GX2TessellationMode::Value tessMode;
   GX2AttribStream attribs[1];
};
