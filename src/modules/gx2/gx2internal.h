#pragma once
#include "gx2.h"
#include "gx2_draw.h"
#include "gx2_context.h"
#include "gx2_texture.h"
#include "gx2_shaders.h"
#include <gl\GL.h>

static const int NUM_MRT_BUFFER = 8;
static const int NUM_TEXTURE_UNIT = 16;

struct GX2State {
   GX2ContextState *contextState;
   GX2ColorBuffer *colorBuffer[NUM_MRT_BUFFER];
   GX2DepthBuffer *depthBuffer;
   GX2Texture *textureUnit[NUM_TEXTURE_UNIT];

   GX2FetchShader *fetchShader;
   GX2VertexShader *vertexShader;
   GX2PixelShader *pixelShader;

};

GLuint getColorBuffer(GX2ColorBuffer *buffer);

extern GX2State gGX2State;