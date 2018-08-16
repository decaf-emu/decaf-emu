#pragma once
#include "cafe/cafe_ppc_interface_varargs.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2PixelShader;
struct GX2VertexShader;

void
GX2DebugTagUserString(uint32_t unk,
                      virt_ptr<const char> fmt,
                      var_args va);

void
GX2DebugTagUserStringVA(uint32_t unk,
                        virt_ptr<const char> fmt,
                        virt_ptr<va_list> vaList);

namespace internal
{

void
debugDumpTexture(virt_ptr<const GX2Texture> texture);

void
debugDumpShader(virt_ptr<const GX2FetchShader> shader);

void
debugDumpShader(virt_ptr<const GX2PixelShader> shader);

void
debugDumpShader(virt_ptr<const GX2VertexShader> shader);

void
writeDebugMarker(std::string_view key,
                 uint32_t id);

} // namespace internal

} // namespace cafe::gx2
