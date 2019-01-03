#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

namespace internal
{

void
initialiseDebug();

void
debugDumpTexture(virt_ptr<const GX2Texture> texture);

void
debugDumpShader(virt_ptr<const GX2FetchShader> shader);

void
debugDumpShader(virt_ptr<const GX2PixelShader> shader);

void
debugDumpShader(virt_ptr<const GX2VertexShader> shader);

} // namespace internal

} // namespace cafe::gx2
