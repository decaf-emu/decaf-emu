#pragma once
#include <cstdint>
#include "ppcutils/va_list.h"

namespace gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

void
GX2DebugTagUserString(uint32_t unk,
                      const char *fmt,
                      ppctypes::VarArgs);

void
GX2DebugTagUserStringVA(uint32_t unk,
                        const char *fmt,
                        ppctypes::va_list *list);

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
