#pragma once
#include <cstdint>

namespace gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

namespace internal
{

void
debugDumpTexture(const GX2Texture *texture);

void
debugDumpShader(GX2FetchShader *shader);

void
debugDumpShader(GX2PixelShader *shader);

void
debugDumpShader(GX2VertexShader *shader);

void writeDebugMarker(const char *key, uint32_t id);

} // namespace internal

} // namespace gx2
