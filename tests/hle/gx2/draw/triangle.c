#include <gfd.h>
#include <coreinit/memdefaultheap.h>
#include <gx2/draw.h>
#include <gx2/shaders.h>
#include <gx2r/draw.h>
#include <gx2r/buffer.h>
#include <string.h>
#include <stdio.h>
#include <whb/file.h>
#include <whb/gfx.h>
#include <whb/proc.h>
#include <whb/sdcard.h>
#include <whb/log.h>
#include <whb/log_udp.h>

static const float sPositionData[] =
{
    0.0f, -0.5f, 0.0f, 0.0f,
   -0.5f,  0.5f, 0.0f, 0.0f,
    0.5f,  0.5f, 0.0f, 0.0f
};

static const float sColourData[] =
{
   1.0f,  0.0f,  0.0f, 1.0f,
   0.0f,  1.0f,  0.0f, 1.0f,
   0.0f,  0.0f,  1.0f, 1.0f
};

int main(int argc, char **argv)
{
   GX2RBuffer positionBuffer = { 0 };
   GX2RBuffer colourBuffer = { 0 };
   WHBGfxShaderGroup group = { 0 };
   void *buffer = NULL;
   char *gshFileData = NULL;
   char *sdRootPath = NULL;
   char path[256];
   int result = 0;

   WHBLogUdpInit();
   WHBProcInit();
   WHBGfxInit();

   if (!WHBMountSdCard()) {
      result = -1;
      goto exit;
   }

   sdRootPath = WHBGetSdCardMountPath();
   sprintf(path, "%s/decaf/shaders/pos_colour.gsh", sdRootPath);

   gshFileData = WHBReadWholeFile(path, NULL);
   if (!WHBGfxLoadGFDShaderGroup(&group, 0, gshFileData)) {
      result = -1;
      WHBLogPrintf("WHBGfxLoadGFDShaderGroup returned  FALSE");
      goto exit;
   }

   WHBGfxInitShaderAttribute(&group, "aPosition", 0, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
   WHBGfxInitShaderAttribute(&group, "aColour", 1, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);
   WHBGfxInitFetchShader(&group);

   WHBFreeWholeFile(gshFileData);
   gshFileData = NULL;

   positionBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
   positionBuffer.elemSize = 4 * 4;
   positionBuffer.elemCount = 3;
   GX2RCreateBuffer(&positionBuffer);
   buffer = GX2RLockBufferEx(&positionBuffer, 0);
   memcpy(buffer, sPositionData, positionBuffer.elemSize * positionBuffer.elemCount);
   GX2RUnlockBufferEx(&positionBuffer, 0);

   colourBuffer.flags = GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE | GX2R_RESOURCE_USAGE_GPU_READ;
   colourBuffer.elemSize = 4 * 4;
   colourBuffer.elemCount = 3;
   GX2RCreateBuffer(&colourBuffer);
   buffer = GX2RLockBufferEx(&colourBuffer, 0);
   memcpy(buffer, sColourData, colourBuffer.elemSize * colourBuffer.elemCount);
   GX2RUnlockBufferEx(&colourBuffer, 0);

   WHBLogPrintf("Begin rendering ...");
   while (WHBProcIsRunning()) {
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

exit:
   WHBLogPrintf("Exiting...");
   GX2RDestroyBufferEx(&positionBuffer, 0);
   GX2RDestroyBufferEx(&colourBuffer, 0);
   //WHBUnmountSdCard(); !! freezes on unmount for unknown reason !!
   WHBGfxShutdown();
   WHBProcShutdown();
   return result;
}
