#pragma once
#ifdef DECAF_VULKAN
#include <cstdint>
#include <common/align.h>

namespace spirv
{

struct Vec4
{
   float x;
   float y;
   float z;
   float w;
};

struct VertexPushConstants
{
   Vec4 posMulAdd;
   Vec4 zSpaceMul;
   float pointSize;
};
static constexpr int VertexPushConstantsSize = sizeof(VertexPushConstants);
static constexpr int VertexPushConstantsOffset = 0;

struct FragmentPushConstants
{
   uint32_t alphaFunc;
   float alphaRef;
   uint32_t needsPremultiply;
};
static constexpr int FragmentPushConstantsSize = sizeof(FragmentPushConstants);
static constexpr int FragmentPushConstantsOffset = align_up<int>(VertexPushConstantsOffset + VertexPushConstantsSize, 16);

} // namespace spirv

#endif
