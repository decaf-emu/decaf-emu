#pragma once
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

namespace GX2AttribIndexType
{
enum Type : uint32_t
{
   PerVertex = 0,
   PerInstance = 1,
   First = 0,
   Last = 1,
};
}

namespace GX2EndianSwapMode
{
enum Mode : uint32_t
{
   None = 0,
   First = 0,
   Last = 3,
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

struct GX2FetchShader
{
   DriverData<4> driverData;
   be_val<uint32_t> size;
   be_ptr<void> data;
   be_val<uint32_t> attribCount;
   UNKNOWN(12);
};
CHECK_OFFSET(GX2FetchShader, 0x0, driverData);
CHECK_OFFSET(GX2FetchShader, 0x4, size);
CHECK_OFFSET(GX2FetchShader, 0x8, data);
CHECK_OFFSET(GX2FetchShader, 0xc, attribCount);
CHECK_SIZE(GX2FetchShader, 0x1c);

struct GX2VertexShader
{
   DriverData<208> driverData;
   be_val<uint32_t> size;
   be_ptr<void> data;

};
CHECK_OFFSET(GX2VertexShader, 0x0, driverData);
CHECK_OFFSET(GX2VertexShader, 0xd0, size);
CHECK_OFFSET(GX2VertexShader, 0xd4, data);

struct GX2PixelShader
{
   DriverData<164> driverData;
   be_val<uint32_t> size;
   be_ptr<void> data;

};
CHECK_OFFSET(GX2PixelShader, 0x0, driverData);
CHECK_OFFSET(GX2PixelShader, 0xa4, size);
CHECK_OFFSET(GX2PixelShader, 0xa8, data);

struct GX2GeometryShader;
struct GX2PixelSampler;

#pragma pack(push, 1)

struct GX2AttribStream
{
   be_val<uint32_t> location;
   be_val<uint32_t> buffer;
   be_val<uint32_t> offset;
   be_val<GX2AttribFormat::Format> format;
   be_val<GX2AttribIndexType::Type> type;
   be_val<uint32_t> aluDivisor;
   be_val<uint32_t> mask;
   be_val<GX2EndianSwapMode::Mode> endianSwap;
};
CHECK_OFFSET(GX2AttribStream, 0x0, location);
CHECK_OFFSET(GX2AttribStream, 0x4, buffer);
CHECK_OFFSET(GX2AttribStream, 0x8, offset);
CHECK_OFFSET(GX2AttribStream, 0xc, format);
CHECK_OFFSET(GX2AttribStream, 0x10, type);
CHECK_OFFSET(GX2AttribStream, 0x14, aluDivisor);
CHECK_OFFSET(GX2AttribStream, 0x18, mask);
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

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader);

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader);

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader);

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader);
