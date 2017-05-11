#include <gfd.h>
#include <defaultheap.h>
#include <gx2/draw.h>
#include <gx2/shaders.h>
#include <gx2r/draw.h>
#include <gx2r/buffer.h>
#include <string.h>
#include <whb/file.h>
#include <whb/gfx.h>
#include <whb/proc.h>

static const float sPositionData[] =
{
    0.0f, -0.5f, 0.0f,
   -0.5f,  0.5f, 0.0f,
    0.5f,  0.5f, 0.0f
};

static const float sColourData[] =
{
   1.0f,  0.0f,  0.0f,
   0.0f,  1.0f,  0.0f,
   0.0f,  0.0f,  1.0f
};

int main(int argc, char **argv)
{
   GX2RBuffer positionBuffer = { 0 };
   GX2RBuffer colourBuffer = { 0 };
   WHBGfxShaderGroup group = { 0 };
   void *buffer = NULL;
   char *gshFileData = NULL;

   WHBProcInit();
   WHBGfxInit();

   gshFileData = WHBReadWholeFile("shaders/pos_colour.gsh", NULL);

   if (!WHBGfxLoadGFDShaderGroup(&group, 0, gshFileData)) {
      return -1;
   }

   WHBGfxInitShaderAttribute(&group, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
   WHBGfxInitShaderAttribute(&group, "aColour", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32);
   WHBGfxInitFetchShader(&group);

   WHBFreeWholeFile(gshFileData);
   gshFileData = NULL;

   positionBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
   positionBuffer.elemSize = 4 * 3;
   positionBuffer.elemCount = 3;
   GX2RCreateBuffer(&positionBuffer);
   buffer = GX2RLockBufferEx(&positionBuffer, 0);
   memcpy(buffer, sPositionData, 4 * 3 * 3);
   GX2RUnlockBufferEx(&positionBuffer, 0);

   colourBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
   colourBuffer.elemSize = 4 * 3;
   colourBuffer.elemCount = 3;
   GX2RCreateBuffer(&colourBuffer);
   buffer = GX2RLockBufferEx(&colourBuffer, 0);
   memcpy(buffer, sColourData, 4 * 3 * 3);
   GX2RUnlockBufferEx(&colourBuffer, 0);

   while(WHBProcIsRunning()) {
      WHBGfxBeginRender();

      WHBGfxBeginRenderTV();
      GX2SetFetchShader(&group.fetchShader);
      GX2SetVertexShader(group.vertexShader);
      GX2SetPixelShader(group.pixelShader);
      GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
      GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
      GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
      WHBGfxFinishRenderTV();

      WHBGfxBeginRenderDRC();
      GX2SetFetchShader(&group.fetchShader);
      GX2SetVertexShader(group.vertexShader);
      GX2SetPixelShader(group.pixelShader);
      GX2RSetAttributeBuffer(&positionBuffer, 0, positionBuffer.elemSize, 0);
      GX2RSetAttributeBuffer(&colourBuffer, 1, colourBuffer.elemSize, 0);
      GX2DrawEx(GX2_PRIMITIVE_MODE_TRIANGLES, 3, 0, 1);
      WHBGfxFinishRenderDRC();

      WHBGfxFinishRender();
   }

   WHBGfxShutdown();
   WHBProcShutdown();
   return 0;
}
