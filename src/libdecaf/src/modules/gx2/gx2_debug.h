#pragma once
#include <cstdint>

namespace gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

void
GX2DebugDumpTexture(const GX2Texture *texture);

void
GX2DebugDumpShader(GX2FetchShader *shader);

void
GX2DebugDumpShader(GX2PixelShader *shader);

void
GX2DebugDumpShader(GX2VertexShader *shader);

namespace internal
{

void writeDebugMarker(const char *key, uint32_t id);

} // namespace internal

} // namespace gx2
