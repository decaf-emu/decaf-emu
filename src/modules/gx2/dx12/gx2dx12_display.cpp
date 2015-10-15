#include "modules/gx2/gx2.h"

#ifdef GX2_DX12
#include "dx12_state.h"
#include "dx12_scanbuffer.h"
#include "dx12_colorbuffer.h"
#include "dx12_depthbuffer.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_time.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/gx2/gx2_display.h"
#include "modules/gx2/gx2_vsync.h"
#include "platform.h"

static uint32_t
gSwapInterval = 0;

static BOOL
gTVEnable = TRUE;

static BOOL
gDRCEnable = TRUE;

void
GX2SetTVEnable(BOOL enable)
{
   gTVEnable = enable;
}

void
GX2SetDRCEnable(BOOL enable)
{
   gDRCEnable = enable;
}

void
GX2CalcTVSize(GX2TVRenderMode::Mode tvRenderMode,
              GX2SurfaceFormat::Format surfaceFormat,
              GX2BufferingMode::Mode bufferingMode,
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut)
{
   // TODO: GX2CalcTVSize
   *size = 1920 * 1080;
}

void
GX2CalcDRCSize(GX2DrcRenderMode::Mode drcRenderMode,
               GX2SurfaceFormat::Format surfaceFormat,
               GX2BufferingMode::Mode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   // TODO: GX2CalcDRCSize
   *size = 854 * 480;
}

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode::Mode tvRenderMode,
               GX2SurfaceFormat::Format surfaceFormat,
               GX2BufferingMode::Mode bufferingMode)
{
   // TODO: GX2SetTVBuffer
   if (!gDX.tvScanBuffer) {
      gDX.tvScanBuffer = new DXScanBufferData();
   } else {
      gDX.tvScanBuffer->release();
   }

   int tvWidth = 0;
   int tvHeight = 0;

   if (tvRenderMode == GX2TVRenderMode::STD480p) {
      tvWidth = 640;
      tvHeight = 480;
   } else if (tvRenderMode == GX2TVRenderMode::WIDE480p) {
      tvWidth = 704;
      tvHeight = 480;
   } else if (tvRenderMode == GX2TVRenderMode::WIDE720p) {
      tvWidth = 1280;
      tvHeight = 720;
   } else if (tvRenderMode == GX2TVRenderMode::WIDE1080p) {
      tvWidth = 1920;
      tvHeight = 1080;
   } else {
      assert(0);
   }

   gDX.tvScanBuffer->alloc(tvWidth, tvHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void
GX2SetDRCBuffer(void *buffer, uint32_t size,
                GX2DrcRenderMode::Mode drcRenderMode,
                GX2SurfaceFormat::Format surfaceFormat,
                GX2BufferingMode::Mode bufferingMode)
{
   // TODO: GX2SetDRCBuffer
   if (!gDX.drcScanBuffer) {
      gDX.drcScanBuffer = new DXScanBufferData();
   } else {
      gDX.drcScanBuffer->release();
   }

   int drcWidth = 854;
   int drcHeight = 480;

   gDX.drcScanBuffer->alloc(drcWidth, drcHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void
GX2SetTVScale(uint32_t x, uint32_t y)
{
   // TODO: GX2SetTVScale
}

void
GX2SetDRCScale(uint32_t x, uint32_t y)
{
   // TODO: GX2SetDRCScale
}

uint32_t
GX2GetSwapInterval()
{
   return gSwapInterval;
}

void
GX2SetSwapInterval(uint32_t interval)
{
   gSwapInterval = interval;
}

GX2TVScanMode::Mode
GX2GetSystemTVScanMode()
{
   // TODO: GX2GetSystemTVScanMode
   return GX2TVScanMode::Last;
}

BOOL
GX2DrawDone()
{
   // TODO: GX2DrawDone
   return TRUE;
}

void
GX2SwapScanBuffers()
{
   dx::renderScanBuffers();
}

void
GX2WaitForFlip()
{
   // TODO: GX2WaitForFlip
}

void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync)
{
   const uint64_t fenceValue = gDX.fence->GetCompletedValue();

   *swapCount = gDX.swapCount;
   *flipCount = fenceValue;

   // TODO: Implement time values for GX2GetSwapStatus
   //*pLastFlip = OSGetTime();
   *lastVsync = gLastVsync;
}

BOOL
GX2GetLastFrame(GX2ScanTarget::Target scanTarget,
                GX2Texture *texture)
{
   // TODO: GX2GetLastFrame
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Target scanTarget,
                     be_val<float> *gamma)
{
   // TODO: GX2GetLastFrameGamma
   return FALSE;
}

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer,
                               GX2ScanTarget::Target scanTarget)
{
   // TODO: GX2CopyColorBufferToScanBuffer

   auto hostColorBuffer = dx::getColorBuffer(buffer);

   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hostColorBuffer->buffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

   if (scanTarget == GX2ScanTarget::TV) {

      CD3DX12_TEXTURE_COPY_LOCATION dest(gDX.tvScanBuffer->buffer.Get(), 0);
      CD3DX12_TEXTURE_COPY_LOCATION source(hostColorBuffer->buffer.Get(), 0);
      gDX.commandList->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);

   } else if (scanTarget == GX2ScanTarget::DRC) {

      CD3DX12_TEXTURE_COPY_LOCATION dest(gDX.drcScanBuffer->buffer.Get(), 0);
      CD3DX12_TEXTURE_COPY_LOCATION source(hostColorBuffer->buffer.Get(), 0);
      gDX.commandList->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);

   }

   gDX.commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(hostColorBuffer->buffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
}

#endif
