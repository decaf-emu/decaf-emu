#pragma once

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
