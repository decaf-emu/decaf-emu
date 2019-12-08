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

struct alignas(16) VertexPushConstants
{
   Vec4 posMulAdd;
   Vec4 zSpaceMul;
   float pointSize;
};
static constexpr int VertexPushConstantsSize = sizeof(VertexPushConstants);
static constexpr int VertexPushConstantsOffset = 0;

struct alignas(16) FragmentPushConstants
{
   uint32_t alphaFunc;
   float alphaRef;
   uint32_t needsPremultiply;
};
static constexpr int FragmentPushConstantsSize = sizeof(FragmentPushConstants);
static constexpr int FragmentPushConstantsOffset = VertexPushConstantsSize;

// Vulkan only requires 4 byte alignment but it seems MoltenVK wants us to
// align to 16 bytes.
static_assert((VertexPushConstantsSize % 16) == 0);
static_assert((VertexPushConstantsOffset % 16) == 0);

static_assert((FragmentPushConstantsSize % 16) == 0);
static_assert((FragmentPushConstantsOffset % 16) == 0);

} // namespace spirv

#endif
