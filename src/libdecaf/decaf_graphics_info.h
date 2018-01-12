#pragma once

#include <cstdint>

namespace decaf
{

struct GraphicsDebugInfoGL {
   uint64_t fetchShaders;
   uint64_t vertexShaders;
   uint64_t pixelShaders;

   uint64_t shaderPipelines;
   uint64_t surfaces;
   uint64_t dataBuffers;

   // TODO: Samplers?
};

} // namespace decaf
