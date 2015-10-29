#pragma once
#include "modules/gx2/gx2_enum.h"
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"
#include "gx2_sampler.h"

struct GX2FetchShader
{
   UNKNOWN(4);
   be_val<uint32_t> size;
   be_ptr<void> data;
   be_val<uint32_t> attribCount;
   UNKNOWN(12);
};
CHECK_OFFSET(GX2FetchShader, 0x4, size);
CHECK_OFFSET(GX2FetchShader, 0x8, data);
CHECK_OFFSET(GX2FetchShader, 0xc, attribCount);
CHECK_SIZE(GX2FetchShader, 0x1c);

struct GX2UniformVar
{
   be_ptr<const char> name;
   be_val<GX2UniformType::Value> type;
   be_val<uint32_t> count;
   be_val<uint32_t> offset;
   be_val<int32_t> block;
};
CHECK_OFFSET(GX2UniformVar, 0x00, name);
CHECK_OFFSET(GX2UniformVar, 0x04, type);
CHECK_OFFSET(GX2UniformVar, 0x08, count);
CHECK_OFFSET(GX2UniformVar, 0x0C, offset);
CHECK_OFFSET(GX2UniformVar, 0x10, block);
CHECK_SIZE(GX2UniformVar, 0x14);

struct GX2UniformBlock
{
   be_ptr<const char> name;
   be_val<uint32_t> offset;
   be_val<uint32_t> size;
};
CHECK_OFFSET(GX2UniformBlock, 0x00, name);
CHECK_OFFSET(GX2UniformBlock, 0x04, offset);
CHECK_OFFSET(GX2UniformBlock, 0x08, size);
CHECK_SIZE(GX2UniformBlock, 0x0C);

struct GX2SamplerVar;

struct GX2VertexShader
{
   UNKNOWN(208);
   be_val<uint32_t> size;
   be_ptr<uint8_t> data;
   be_val<GX2ShaderMode::Value> mode;

   be_val<uint32_t> uniformBlockCount;
   be_ptr<GX2UniformBlock> uniformBlocks;

   be_val<uint32_t> uniformVarCount;
   be_ptr<GX2UniformVar> uniformVars;

   be_val<uint32_t> numUnk1;  // Size of unk1
   be_ptr<void> unk1;         // Array of something

   be_val<uint32_t> numUnk2;  // Size of unk2
   be_ptr<void> unk2;         // Array of something

   be_val<uint32_t> samplerVarCount;
   be_ptr<GX2SamplerVar> samplerVars;

   be_val<uint32_t> numUnk3;  // Size of unk3
   be_ptr<void> unk3;         // Array of something

   UNKNOWN(4 * 10);
};
CHECK_OFFSET(GX2VertexShader, 0xd0, size);
CHECK_OFFSET(GX2VertexShader, 0xd4, data);
CHECK_OFFSET(GX2VertexShader, 0xd8, mode);
CHECK_OFFSET(GX2VertexShader, 0xdc, uniformBlockCount);
CHECK_OFFSET(GX2VertexShader, 0xe0, uniformBlocks);
CHECK_OFFSET(GX2VertexShader, 0xe4, uniformVarCount);
CHECK_OFFSET(GX2VertexShader, 0xe8, uniformVars);
CHECK_OFFSET(GX2VertexShader, 0xfc, samplerVarCount);
CHECK_OFFSET(GX2VertexShader, 0x100, samplerVars);
CHECK_SIZE(GX2VertexShader, 0x134);

struct GX2PixelShader
{
   UNKNOWN(164);
   be_val<uint32_t> size;
   be_ptr<uint8_t> data;
   be_val<GX2ShaderMode::Value> mode;

   be_val<uint32_t> uniformBlockCount;
   be_ptr<GX2UniformBlock> uniformBlocks;

   be_val<uint32_t> uniformVarCount;
   be_ptr<GX2UniformVar> uniformVars;

   be_val<uint32_t> numUnk1;  // Size of unk1
   be_ptr<void> unk1;         // Array of something

   be_val<uint32_t> numUnk2;  // Size of unk2
   be_ptr<void> unk2;         // Array of something

   be_val<uint32_t> samplerVarCount;
   be_ptr<GX2SamplerVar> samplerVars;

   be_val<uint32_t> unk3;
   be_val<uint32_t> unk4;
   be_val<uint32_t> unk5;
   be_val<uint32_t> unk6;
};
CHECK_OFFSET(GX2PixelShader, 0xa4, size);
CHECK_OFFSET(GX2PixelShader, 0xa8, data);
CHECK_OFFSET(GX2PixelShader, 0xac, mode);
CHECK_OFFSET(GX2PixelShader, 0xb0, uniformBlockCount);
CHECK_OFFSET(GX2PixelShader, 0xb4, uniformBlocks);
CHECK_OFFSET(GX2PixelShader, 0xb8, uniformVarCount);
CHECK_OFFSET(GX2PixelShader, 0xbc, uniformVars);
CHECK_OFFSET(GX2PixelShader, 0xd0, samplerVarCount);
CHECK_OFFSET(GX2PixelShader, 0xd4, samplerVars);
CHECK_SIZE(GX2PixelShader, 0xe8);

struct GX2GeometryShader;

#pragma pack(push, 1)

struct GX2AttribStream
{
   be_val<uint32_t> location;
   be_val<uint32_t> buffer;
   be_val<uint32_t> offset;
   be_val<GX2AttribFormat::Value> format;
   be_val<GX2AttribIndexType::Value> type;
   be_val<uint32_t> aluDivisor;
   be_val<uint32_t> mask;
   be_val<GX2EndianSwapMode::Value> endianSwap;
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
GX2CalcFetchShaderSizeEx(uint32_t attribs,
                         GX2FetchShaderType::Value fetchShaderType,
                         GX2TessellationMode::Value tesellationMode);

void
GX2InitFetchShaderEx(GX2FetchShader *fetchShader,
                     void *buffer,
                     uint32_t count,
                     GX2AttribStream *attribs,
                     GX2FetchShaderType::Value type,
                     GX2TessellationMode::Value tessMode);

void
GX2SetFetchShader(GX2FetchShader *shader);

void
GX2SetVertexShader(GX2VertexShader *shader);

void
GX2SetPixelShader(GX2PixelShader *shader);

void
GX2SetGeometryShader(GX2GeometryShader *shader);

void
GX2SetPixelSampler(GX2Sampler *sampler, uint32_t id);

void
GX2SetVertexUniformReg(uint32_t offset, uint32_t count, void *data);

void
GX2SetPixelUniformReg(uint32_t offset, uint32_t count, void *data);

void
GX2SetVertexUniformBlock(uint32_t location, uint32_t size, const void *data);

void
GX2SetPixelUniformBlock(uint32_t location, uint32_t size, const void *data);

void
GX2SetShaderModeEx(GX2ShaderMode::Value mode,
                   uint32_t unk1, uint32_t unk2, uint32_t unk3,
                   uint32_t unk4, uint32_t unk5, uint32_t unk6);

uint32_t
GX2GetPixelShaderGPRs(GX2PixelShader *shader);

uint32_t
GX2GetPixelShaderStackEntries(GX2PixelShader *shader);

uint32_t
GX2GetVertexShaderGPRs(GX2VertexShader *shader);

uint32_t
GX2GetVertexShaderStackEntries(GX2VertexShader *shader);
