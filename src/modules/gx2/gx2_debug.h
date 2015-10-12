#pragma once

struct GX2Texture;
struct GX2PixelShader;
struct GX2VertexShader;

void
GX2DumpTexture(const GX2Texture *texture);

void
GX2DumpShader(GX2PixelShader *shader);

void
GX2DumpShader(GX2VertexShader *shader);
