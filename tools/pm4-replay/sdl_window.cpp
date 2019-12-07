#include "sdl_window.h"
#include "clilog.h"
#include "config.h"

#include "replay_ringbuffer.h"
#include "replay_parser_pm4.h"

#include <array>
#include <atomic>
#include <fstream>
#include <common/log.h>
#include <common/platform_dir.h>
#include <future>
#include <libgpu/gpu_config.h>
#include <SDL_syswm.h>

using namespace latte::pm4;

RingBuffer *sRingBuffer = nullptr; // eww is global because of onGpuInterrupt

void initialiseRegisters(RingBuffer *ringBuffer);

SDLWindow::~SDLWindow()
{
}

bool
SDLWindow::initCore()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   return true;
}

bool
SDLWindow::initGraphics()
{
   auto videoInitialised = false;

#ifdef SDL_VIDEO_DRIVER_X11
   if (!videoInitialised) {
      videoInitialised = SDL_VideoInit("x11") == 0;
      if (!videoInitialised) {
         gCliLog->error("Failed to initialize SDL Video with x11: {}", SDL_GetError());
      }
   }
#endif

#ifdef SDL_VIDEO_DRIVER_WAYLAND
   if (!videoInitialised) {
      videoInitialised = SDL_VideoInit("wayland") == 0;
      if (!videoInitialised) {
         gCliLog->error("Failed to initialize SDL Video with wayland: {}", SDL_GetError());
      }
   }
#endif

   if (!videoInitialised) {
      if (SDL_VideoInit(NULL) != 0) {
         gCliLog->error("Failed to initialize SDL Video: {}", SDL_GetError());
         return false;
      }
   }

   gCliLog->info("Using SDL video driver {}", SDL_GetCurrentVideoDriver());

   mGraphicsDriver = gpu::createGraphicsDriver();
   if (!mGraphicsDriver) {
      return false;
   }

   switch (mGraphicsDriver->type()) {
   case gpu::GraphicsDriverType::Vulkan:
      mRendererName = "Vulkan";
      break;
   case gpu::GraphicsDriverType::Null:
      mRendererName = "Null";
      break;
   default:
      mRendererName = "Unknown";
   }

   return true;
}

static void
onGpuInterrupt()
{
   auto entries = gpu::ih::read();
   sRingBuffer->onGpuInterrupt();
}

bool
SDLWindow::run(const std::string &tracePath)
{
   std::atomic_bool shouldQuit = false;

   // Setup some basic window stuff
   mWindow =
      SDL_CreateWindow("pm4-replay",
                       SDL_WINDOWPOS_UNDEFINED,
                       SDL_WINDOWPOS_UNDEFINED,
                       WindowWidth, WindowHeight,
                       SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (gpu::config()->display.screenMode == gpu::DisplaySettings::Fullscreen) {
      SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
   }

   // Setup graphics driver
   auto wsi = gpu::WindowSystemInfo { };
   auto sysWmInfo = SDL_SysWMinfo { };
   SDL_VERSION(&sysWmInfo.version);
   if (!SDL_GetWindowWMInfo(mWindow, &sysWmInfo)) {
      gCliLog->error("SDL_GetWindowWMInfo failed: {}", SDL_GetError());
   }

   switch (sysWmInfo.subsystem) {
#ifdef SDL_VIDEO_DRIVER_WINDOWS
   case SDL_SYSWM_WINDOWS:
      wsi.type = gpu::WindowSystemType::Windows;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.win.window);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_X11
   case SDL_SYSWM_X11:
      wsi.type = gpu::WindowSystemType::X11;
      wsi.renderSurface = reinterpret_cast<void *>(sysWmInfo.info.x11.window);
      wsi.displayConnection = static_cast<void *>(sysWmInfo.info.x11.display);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_COCOA
   case SDL_SYSWM_COCOA:
      wsi.type = gpu::WindowSystemType::Cocoa;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.cocoa.window);
      break;
#endif
#ifdef SDL_VIDEO_DRIVER_WAYLAND
   case SDL_SYSWM_WAYLAND:
      wsi.type = gpu::WindowSystemType::Wayland;
      wsi.renderSurface = static_cast<void *>(sysWmInfo.info.wl.surface);
      wsi.displayConnection = static_cast<void *>(sysWmInfo.info.wl.display);
      break;
#endif
   default:
      decaf_abort(fmt::format("Unsupported SDL window subsystem {}", sysWmInfo.subsystem));
   }

   mGraphicsDriver->setWindowSystemInfo(wsi);

   // Setup replay parser
   auto replayHeap = phys_cast<cafe::TinyHeapPhysical *>(phys_addr { 0x34000000 });
   cafe::TinyHeap_Setup(replayHeap,
                        0x430,
                        phys_cast<void *>(phys_addr { 0x34000000 + 0x430 }),
                        0x1C000000 - 0x430);
   auto ringBuffer = std::make_unique<RingBuffer>(replayHeap);
   sRingBuffer = ringBuffer.get();

   initialiseRegisters(ringBuffer.get());

   auto parser = ReplayParserPM4::Create(mGraphicsDriver, ringBuffer.get(),
                                         replayHeap, tracePath);
   if (!parser) {
      return false;
   }

   gpu::ih::enable(latte::CP_INT_CNTL::get(0xFFFFFFFF));
   gpu::ih::setInterruptCallback(onGpuInterrupt);

   auto loopReplay = true;
   auto replayThread = std::thread {
      [&]() {
         do {
            parser->runUntilTimestamp(0xFFFFFFFFFFFFFFFFull);
         } while (loopReplay && !shouldQuit);
      } };

   auto graphicsThread = std::thread {
      [&]() {
         mGraphicsDriver->run();
      } };

   while (!shouldQuit) {
      auto event = SDL_Event { };

      while (SDL_PollEvent(&event)) {
         switch (event.type) {
         case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
               shouldQuit = true;
            }

            break;
         case SDL_KEYUP:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
               shouldQuit = true;
            }
            break;
         case SDL_QUIT:
            shouldQuit = true;
            break;
         }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }

   // We have to wait until replay is finished...
   replayThread.join();

   mGraphicsDriver->stop();
   graphicsThread.join();
   return true;
}

void initialiseRegisters(RingBuffer *ringBuffer)
{
   // Straight copied from gx2

   std::array<uint32_t, 24> zeroes;
   zeroes.fill(0);

   uint32_t values28030_28034[] = {
      latte::PA_SC_SCREEN_SCISSOR_TL::get(0).value,
      latte::PA_SC_SCREEN_SCISSOR_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192).value
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_SC_SCREEN_SCISSOR_TL,
      gsl::make_span(values28030_28034)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_SC_LINE_CNTL,
      latte::PA_SC_LINE_CNTL::get(0)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_SU_VTX_CNTL,
      latte::PA_SU_VTX_CNTL::get(0)
         .PIX_CENTER(latte::PA_SU_VTX_CNTL_PIX_CENTER::OGL)
         .ROUND_MODE(latte::PA_SU_VTX_CNTL_ROUND_MODE::TRUNCATE)
         .QUANT_MODE(latte::PA_SU_VTX_CNTL_QUANT_MODE::QUANT_1_256TH)
         .value
                         });

   // PA_CL_POINT_X_RAD, PA_CL_POINT_Y_RAD, PA_CL_POINT_POINT_SIZE, PA_CL_POINT_POINT_CULL_RAD
   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_CL_POINT_X_RAD,
      gsl::make_span(zeroes.data(), 4)
                         });

   // PA_CL_UCP_0_X ... PA_CL_UCP_5_W
   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_CL_UCP_0_X,
      gsl::make_span(zeroes.data(), 24)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_CL_VTE_CNTL,
      latte::PA_CL_VTE_CNTL::get(0)
         .VPORT_X_SCALE_ENA(true)
         .VPORT_X_OFFSET_ENA(true)
         .VPORT_Y_SCALE_ENA(true)
         .VPORT_Y_OFFSET_ENA(true)
         .VPORT_Z_SCALE_ENA(true)
         .VPORT_Z_OFFSET_ENA(true)
         .VTX_W0_FMT(true)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_CL_NANINF_CNTL,
      latte::PA_CL_NANINF_CNTL::get(0)
         .value
                         });

   uint32_t values28200_28208[] = {
      0,
      latte::PA_SC_WINDOW_SCISSOR_TL::get(0)
         .WINDOW_OFFSET_DISABLE(true)
         .value,
      latte::PA_SC_WINDOW_SCISSOR_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192)
         .value,
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_SC_WINDOW_OFFSET,
      gsl::make_span(values28200_28208)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_SC_LINE_STIPPLE,
      latte::PA_SC_LINE_STIPPLE::get(0)
      .value
                         });

   uint32_t values28A0C_28A10[] = {
      latte::PA_SC_MPASS_PS_CNTL::get(0)
         .value,
      latte::PA_SC_MODE_CNTL::get(0)
         .MSAA_ENABLE(true)
         .FORCE_EOV_CNTDWN_ENABLE(true)
         .FORCE_EOV_REZ_ENABLE(true)
         .value
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_SC_LINE_STIPPLE,
      gsl::make_span(values28A0C_28A10)
                         });

   uint32_t values28250_28254[] = {
      latte::PA_SC_VPORT_SCISSOR_0_TL::get(0)
         .WINDOW_OFFSET_DISABLE(true)
         .value,
      latte::PA_SC_VPORT_SCISSOR_0_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192)
         .value,
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::PA_SC_VPORT_SCISSOR_0_TL,
      gsl::make_span(values28250_28254)
                         });

   // TODO: Register 0x8B24 unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x8B24),
      0xFF3FFF
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_SC_CLIPRECT_RULE,
      latte::PA_SC_CLIPRECT_RULE::get(0)
         .CLIP_RULE(0xFFFF)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_GS_VERTEX_REUSE,
      latte::VGT_GS_VERTEX_REUSE::get(0)
         .VERT_REUSE(16)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_OUTPUT_PATH_CNTL,
      latte::VGT_OUTPUT_PATH_CNTL::get(0)
         .PATH_SELECT(latte::VGT_OUTPUT_PATH_SELECT::TESS_EN)
         .value
                         });

   // TODO: This is an unknown value 16 * 0xb14(r31) * 0xb18(r31)
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_ES_PER_GS,
      latte::VGT_ES_PER_GS::get(0)
         .ES_PER_GS(16 * 1 * 1)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_GS_PER_ES,
      latte::VGT_GS_PER_ES::get(0)
         .GS_PER_ES(256)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::VGT_GS_PER_VS,
      latte::VGT_GS_PER_VS::get(0)
         .GS_PER_VS(4)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_INDX_OFFSET,
      latte::VGT_INDX_OFFSET::get(0)
         .INDX_OFFSET(0)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_REUSE_OFF,
      latte::VGT_REUSE_OFF::get(0)
         .REUSE_OFF(false)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_MULTI_PRIM_IB_RESET_EN,
      latte::VGT_MULTI_PRIM_IB_RESET_EN::get(0)
         .RESET_EN(true)
         .value
                         });

   uint32_t values28C58_28C5C[] = {
      latte::VGT_VERTEX_REUSE_BLOCK_CNTL::get(0)
         .VTX_REUSE_DEPTH(14)
         .value,
      latte::VGT_OUT_DEALLOC_CNTL::get(0)
         .DEALLOC_DIST(16)
         .value,
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::VGT_VERTEX_REUSE_BLOCK_CNTL,
      gsl::make_span(values28C58_28C5C)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_HOS_REUSE_DEPTH,
      latte::VGT_HOS_REUSE_DEPTH::get(0)
         .REUSE_DEPTH(16)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_STRMOUT_DRAW_OPAQUE_OFFSET,
      latte::VGT_STRMOUT_DRAW_OPAQUE_OFFSET::get(0)
         .OFFSET(0)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::VGT_VTX_CNT_EN,
      latte::VGT_VTX_CNT_EN::get(0)
         .VTX_CNT_EN(false)
         .value
                         });

   uint32_t values28400_28404[] = {
      latte::VGT_MAX_VTX_INDX::get(0)
         .MAX_INDX(-1)
         .value,
      latte::VGT_MIN_VTX_INDX::get(0)
         .MIN_INDX(0)
         .value
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::VGT_MAX_VTX_INDX,
      gsl::make_span(values28400_28404)
                         });

   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::TA_CNTL_AUX,
      latte::TA_CNTL_AUX::get(0)
         .UNK0(true)
         .SYNC_GRADIENT(true)
         .SYNC_WALKER(true)
         .SYNC_ALIGNER(true)
         .value
                         });

   // TODO: Register 0x9714 unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x9714),
      1
                         });

   // TODO: Register 0x8D8C unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x8D8C),
      0x4000
                         });

   // SQ_ESTMP_RING_BASE ... SQ_REDUC_RING_SIZE
   ringBuffer->writePM4(latte::pm4::SetConfigRegs {
      latte::Register::SQ_ESTMP_RING_BASE,
      gsl::make_span(zeroes.data(), 12)
                         });

   // SQ_ESTMP_RING_ITEMSIZE ... SQ_REDUC_RING_ITEMSIZE
   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::SQ_ESTMP_RING_ITEMSIZE,
      gsl::make_span(zeroes.data(), 6)
                         });

   ringBuffer->writePM4(latte::pm4::SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(0)
         .value
                         });

   // SPI_FOG_CNTL ... SPI_FOG_FUNC_BIAS
   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::SPI_FOG_CNTL,
      gsl::make_span(zeroes.data(), 3)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::SPI_INTERP_CONTROL_0,
      latte::SPI_INTERP_CONTROL_0::get(0)
         .FLAT_SHADE_ENA(true)
         .PNT_SPRITE_ENA(false)
         .PNT_SPRITE_OVRD_X(latte::SPI_PNT_SPRITE_SEL::SEL_S)
         .PNT_SPRITE_OVRD_Y(latte::SPI_PNT_SPRITE_SEL::SEL_T)
         .PNT_SPRITE_OVRD_Z(latte::SPI_PNT_SPRITE_SEL::SEL_0)
         .PNT_SPRITE_OVRD_W(latte::SPI_PNT_SPRITE_SEL::SEL_1)
         .PNT_SPRITE_TOP_1(true)
         .value
                         });

   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      latte::Register::SPI_CONFIG_CNTL_1,
      latte::SPI_CONFIG_CNTL_1::get(0)
         .value
                         });

   // TODO: Register 0x286C8 unknown
   ringBuffer->writePM4(latte::pm4::SetAllContextsReg {
      static_cast<latte::Register>(0x286C8),
      1
                         });

   // TODO: Register 0x28354 unknown
   auto unkValue = 0u; // 0x143C(r31)

   if (unkValue > 0x5270) {
      ringBuffer->writePM4(latte::pm4::SetContextReg {
         static_cast<latte::Register>(0x28354),
         0xFF
                            });
   } else {
      ringBuffer->writePM4(latte::pm4::SetContextReg {
         static_cast<latte::Register>(0x28354),
         0x1FF
                            });
   }

   uint32_t values28D28_28D2C[] = {
      latte::DB_SRESULTS_COMPARE_STATE0::get(0)
         .value,
      latte::DB_SRESULTS_COMPARE_STATE1::get(0)
         .value
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::DB_SRESULTS_COMPARE_STATE0,
      gsl::make_span(values28D28_28D2C)
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::DB_RENDER_OVERRIDE,
      latte::DB_RENDER_OVERRIDE::get(0)
         .value
                         });

   // TODO: Register 0x9830 unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x9830),
      0
                         });

   // TODO: Register 0x983C unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x983C),
      0x1000000
                         });

   uint32_t values28C30_28C3C[] = {
      latte::CB_CLRCMP_CONTROL::get(0)
         .CLRCMP_FCN_SEL(latte::CB_CLRCMP_SEL::SRC)
         .value,
      latte::CB_CLRCMP_SRC::get(0)
         .CLRCMP_SRC(0)
         .value,
      latte::CB_CLRCMP_DST::get(0)
         .CLRCMP_DST(0)
         .value,
      latte::CB_CLRCMP_MSK::get(0)
         .CLRCMP_MSK(0xFFFFFFFF)
         .value
   };

   ringBuffer->writePM4(latte::pm4::SetContextRegs {
      latte::Register::CB_CLRCMP_CONTROL,
      gsl::make_span(values28C30_28C3C)
                         });

   // TODO: Register 0x9A1C unknown
   ringBuffer->writePM4(latte::pm4::SetConfigReg {
      static_cast<latte::Register>(0x9A1C),
      0
                         });

   ringBuffer->writePM4(latte::pm4::SetContextReg {
      latte::Register::PA_SC_AA_MASK,
      latte::PA_SC_AA_MASK::get(0)
         .AA_MASK_ULC(0xFF)
         .AA_MASK_URC(0xFF)
         .AA_MASK_LLC(0xFF)
         .AA_MASK_LRC(0xFF)
         .value
                         });

   // TODO: Register 0x28230 unknown
   ringBuffer->writePM4(latte::pm4::SetContextReg {
      static_cast<latte::Register>(0x28230),
      0xAAAAAAAA
                         });
}
