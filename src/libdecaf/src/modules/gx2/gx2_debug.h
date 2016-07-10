#pragma once
#include <cstdint>
#include "gx2_enum.h"

namespace gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

const char * const GX2DebugTagUserStringTypes[] = {
	"Indent",
	"Undent",
	"Comment",
	"Bookmark"
};

void
GX2DebugDumpTexture(const GX2Texture *texture);

void
GX2DebugDumpShader(GX2FetchShader *shader);

void
GX2DebugDumpShader(GX2PixelShader *shader);

void
GX2DebugDumpShader(GX2VertexShader *shader);

void
GX2DebugTagUserString(GX2DebugTagUserStringType tagType, const char *fmtString, ...);

void
GX2DebugTagUserStringVA(GX2DebugTagUserStringType tagType, const char* fmtString, va_list args);

namespace internal
{

void writeDebugMarker(const char *key, uint32_t id);

} // namespace internal

} // namespace gx2
