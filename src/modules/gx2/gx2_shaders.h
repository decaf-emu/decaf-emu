#include "systemtypes.h"

namespace GX2FetchShaderType
{
enum Type : uint32_t
{
   First = 0,
   Last = 3,
};
}

namespace GX2TessellationMode
{
enum Mode : uint32_t
{
   First = 0,
   Last = 2,
};
}

namespace GX2AttribFormat
{
enum Format : uint32_t
{
   First = 0,
   Last = 0xa0f
};
}

namespace GX2ShaderMode
{
enum Mode : uint32_t
{
   GX2ShaderModeFirst = 0,
   GX2ShaderModeLast = 3
};
}

struct GX2FetchShader;
struct GX2VertexShader;
struct GX2PixelShader;
struct GX2GeometryShader;
struct GX2PixelSampler;

#pragma pack(push, 1)

struct GX2AttribStream
{
   UNKNOWN(0xc);
   be_val<GX2AttribFormat::Format> format;
   UNKNOWN(0x4);
   be_val<uint32_t> aluDivisor;
   UNKNOWN(0x4);
   be_val<uint32_t> endianSwap;
};
CHECK_OFFSET(GX2AttribStream, 0xc, format);
CHECK_OFFSET(GX2AttribStream, 0x14, aluDivisor);
CHECK_OFFSET(GX2AttribStream, 0x1c, endianSwap);
CHECK_SIZE(GX2AttribStream, 0x20);

#pragma pack(pop)

uint32_t
GX2CalcGeometryShaderInputRingBufferSize(uint32_t ringItemSize);

uint32_t
GX2CalcGeometryShaderOutputRingBufferSize(uint32_t ringItemSize);

uint32_t
GX2CalcFetchShaderSizeEx(uint32_t attribs, GX2FetchShaderType::Type fetchShaderType, GX2TessellationMode::Mode tesellationMode);

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader, void *buffer, uint32_t count, GX2AttribStream *attribs, GX2FetchShaderType::Type type, GX2TessellationMode::Mode tessMode);

void
GX2SetFetchShader(GX2FetchShader *shader);

void
GX2SetVertexShader(GX2VertexShader *shader);

void
GX2SetPixelShader(GX2PixelShader *shader);

void
GX2SetGeometryShader(GX2GeometryShader *shader);

void
GX2SetPixelSampler(GX2PixelSampler *sampler, uint32_t id);

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data);

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, void *data);

void
GX2SetShaderModeEx(GX2ShaderMode::Mode mode, uint32_t unk1, uint32_t unk2, uint32_t unk3, uint32_t unk4, uint32_t unk5, uint32_t unk6);
