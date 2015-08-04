#pragma once
#include "../modules/gx2/gx2.h"
#include "../modules/gx2/gx2_draw.h"
#include "../modules/gx2/gx2_context.h"
#include "../modules/gx2/gx2_texture.h"
#include "../modules/gx2/gx2_shaders.h"
#include <glbinding/gl/gl.h>

using namespace gl;

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