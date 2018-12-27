#include "sdl_window.h"
#include "clilog.h"
#include "config.h"

#include <array>
#include <fstream>
#include <common/log.h>
#include <common/platform_dir.h>
#include <common-sdl/decafsdl_config.h>
#include <common-sdl/decafsdl_opengl.h>
#include <common-sdl/decafsdl_vulkan.h>
#include <libcpu/mmu.h>
#include <libcpu/be2_struct.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <libdecaf/decaf_pm4replay.h>
#include <libdecaf/src/cafe/cafe_tinyheap.h>
#include <libgpu/gpu.h>
#include <libgpu/gpu_config.h>
#include <libgpu/gpu_ringbuffer.h>
#include <libgpu/latte/latte_registers.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_reader.h>
#include <libgpu/latte/latte_pm4_writer.h>
#include <libgpu/latte/latte_pm4_sizer.h>

using namespace latte::pm4;

static phys_ptr<cafe::TinyHeapPhysical> sReplayHeap = nullptr;

class RingBuffer
{
   static constexpr auto BufferSize = 0x1000000u;

public:
   RingBuffer()
   {
      auto allocPtr = phys_ptr<void> { nullptr };

      TinyHeap_Alloc(sReplayHeap, BufferSize * 4, 0x100, &allocPtr);
      mBuffer = phys_cast<uint32_t *>(allocPtr);
      mSize = BufferSize;
   }

   void
   clear()
   {
      decaf_check(mWritePosition == mSubmitPosition);
      mWritePosition = 0u;
      mSubmitPosition = 0u;
   }

   void
   flushCommandBuffer()
   {
      gpu::ringbuffer::write({
         mBuffer.getRawPointer() + mSubmitPosition,
         mWritePosition - mSubmitPosition
      });

      mSubmitPosition = mWritePosition;
   }

   template<typename Type>
   void
   writePM4(const Type &value)
   {
      auto &ncValue = const_cast<Type &>(value);

      // Calculate the total size this object will be
      latte::pm4::PacketSizer sizer;
      ncValue.serialise(sizer);
      auto totalSize = sizer.getSize() + 1;

      // Serialize the packet to the active command buffer
      decaf_check(mWritePosition + totalSize < mSize);
      auto writer = latte::pm4::PacketWriter {
         mBuffer.getRawPointer(),
         mWritePosition,
         Type::Opcode,
         totalSize
      };
      ncValue.serialise(writer);
   }

   void
   writeBuffer(void *buffer, uint32_t numWords)
   {
      decaf_check(mWritePosition + numWords < mSize);
      std::memcpy(mBuffer.getRawPointer() + mWritePosition, buffer, numWords * 4);
      mWritePosition += numWords;
   }

private:
   phys_ptr<uint32_t> mBuffer;
   uint32_t mSize = 0u;
   uint32_t mSubmitPosition = 0u;
   uint32_t mWritePosition = 0u;
};

class PM4Parser
{
public:
   PM4Parser(gpu::GraphicsDriver *driver) :
      mGraphicsDriver(driver)
   {
      auto allocPtr = phys_ptr<void> { nullptr };
      TinyHeap_Alloc(sReplayHeap,
                     0x10000 * 4,
                     0x100,
                     &allocPtr);
      mRegisterStorage = phys_cast<uint32_t *>(allocPtr);
   }

   bool open(const std::string &path)
   {
      mFile.open(path, std::ifstream::binary);

      if (!mFile.is_open()) {
         return false;
      }

      std::array<char, 4> magic;
      mFile.read(magic.data(), 4);

      if (magic != decaf::pm4::CaptureMagic) {
         return false;
      }

      return true;
   }

   bool eof()
   {
      return mFile.eof();
   }

   bool readFrame()
   {
      std::vector<char> buffer;
      auto foundSwap = false;

      // Clear ringbuffer
      mRingBuffer.clear();

      while (!foundSwap) {
         decaf::pm4::CapturePacket packet;
         mFile.read(reinterpret_cast<char *>(&packet), sizeof(decaf::pm4::CapturePacket));

         if (!mFile) {
            return false;
         }

         switch (packet.type) {
         case decaf::pm4::CapturePacket::CommandBuffer:
         {
            buffer.resize(packet.size);
            mFile.read(buffer.data(), buffer.size());

            if (!mFile) {
               return false;
            }

            foundSwap |= handleCommandBuffer(buffer.data(), packet.size);
            break;
         }
         case decaf::pm4::CapturePacket::RegisterSnapshot:
         {
            decaf_check((packet.size % 4) == 0);
            auto numRegisters = packet.size / 4;
            mFile.read(reinterpret_cast<char *>(mRegisterStorage.getRawPointer()), packet.size);

            // Swap it into big endian, so we can write LOAD_ commands
            for (auto i = 0u; i < numRegisters; ++i) {
               mRegisterStorage[i] = byte_swap(mRegisterStorage[i]);
            }

            handleRegisterSnapshot(mRegisterStorage, numRegisters);
            mRingBuffer.flushCommandBuffer();
            break;
         }
         case decaf::pm4::CapturePacket::SetBuffer:
         {
            decaf::pm4::CaptureSetBuffer setBuffer;
            mFile.read(reinterpret_cast<char *>(&setBuffer), sizeof(decaf::pm4::CaptureSetBuffer));

            handleSetBuffer(setBuffer);
            mRingBuffer.flushCommandBuffer();
            break;
         }
         case decaf::pm4::CapturePacket::MemoryLoad:
         {
            decaf::pm4::CaptureMemoryLoad load;
            mFile.read(reinterpret_cast<char *>(&load), sizeof(decaf::pm4::CaptureMemoryLoad));

            if (!mFile) {
               return false;
            }

            buffer.resize(packet.size - sizeof(decaf::pm4::CaptureMemoryLoad));
            mFile.read(buffer.data(), buffer.size());

            if (!mFile) {
               return false;
            }

            handleMemoryLoad(load, buffer);
            break;
         }
         default:
            mFile.seekg(packet.size, std::ifstream::cur);
         }
      }

      // Flush ringbuffer to gpu
      mRingBuffer.flushCommandBuffer();
      return foundSwap;
   }

private:
   bool handleCommandBuffer(void *buffer, uint32_t sizeBytes)
   {
      auto numWords = sizeBytes / 4;
      mRingBuffer.writeBuffer(buffer, numWords);
      return scanCommandBuffer(buffer, numWords);
   }

   void handleSetBuffer(decaf::pm4::CaptureSetBuffer &setBuffer)
   {
      auto isTv = (setBuffer.type == decaf::pm4::CaptureSetBuffer::TvBuffer) ? 1u : 0u;

      mRingBuffer.writePM4(DecafSetBuffer {
         latte::pm4::ScanTarget::TV,
         setBuffer.address,
         setBuffer.bufferingMode,
         setBuffer.width,
         setBuffer.height
      });
   }

   void handleRegisterSnapshot(phys_ptr<uint32_t> registers, uint32_t count)
   {
      // Enable loading of registers
      auto LOAD_CONTROL = latte::CONTEXT_CONTROL_ENABLE::get(0)
         .ENABLE_CONFIG_REG(true)
         .ENABLE_CONTEXT_REG(true)
         .ENABLE_ALU_CONST(true)
         .ENABLE_BOOL_CONST(true)
         .ENABLE_LOOP_CONST(true)
         .ENABLE_RESOURCE(true)
         .ENABLE_SAMPLER(true)
         .ENABLE_CTL_CONST(true)
         .ENABLE_ORDINAL(true);

      auto SHADOW_ENABLE = latte::CONTEXT_CONTROL_ENABLE::get(0);

      mRingBuffer.writePM4(ContextControl {
         LOAD_CONTROL,
         SHADOW_ENABLE
      });

      // Write all the register load packets!
      static std::pair<uint32_t, uint32_t>
      LoadConfigRange[] = { { 0, (latte::Register::ConfigRegisterEnd - latte::Register::ConfigRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadConfigReg {
         phys_cast<phys_addr>(registers + (latte::Register::ConfigRegisterBase / 4)),
         gsl::make_span(LoadConfigRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadContextRange[] = { { 0, (latte::Register::ContextRegisterEnd - latte::Register::ContextRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadContextReg {
         phys_cast<phys_addr>(registers + (latte::Register::ContextRegisterBase / 4)),
         gsl::make_span(LoadContextRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadAluConstRange[] = { { 0, (latte::Register::AluConstRegisterEnd - latte::Register::AluConstRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadAluConst {
         phys_cast<phys_addr>(registers + (latte::Register::AluConstRegisterBase / 4)),
         gsl::make_span(LoadAluConstRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadResourceRange[] = { { 0, (latte::Register::ResourceRegisterEnd - latte::Register::ResourceRegisterBase) / 4 }, };

      mRingBuffer.writePM4(latte::pm4::LoadResource {
         phys_cast<phys_addr>(registers + (latte::Register::ResourceRegisterBase / 4)),
         gsl::make_span(LoadResourceRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadSamplerRange[] = { { 0, (latte::Register::SamplerRegisterEnd - latte::Register::SamplerRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadSampler {
         phys_cast<phys_addr>(registers + (latte::Register::SamplerRegisterBase / 4)),
         gsl::make_span(LoadSamplerRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadControlRange[] = { { 0, (latte::Register::ControlRegisterEnd - latte::Register::ControlRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadControlConst {
         phys_cast<phys_addr>(registers + (latte::Register::ControlRegisterBase / 4)),
         gsl::make_span(LoadControlRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadLoopRange[] = { { 0, (latte::Register::LoopConstRegisterEnd - latte::Register::LoopConstRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadLoopConst {
         phys_cast<phys_addr>(registers + (latte::Register::LoopConstRegisterBase / 4)),
         gsl::make_span(LoadLoopRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadBoolRange[] = { { 0, (latte::Register::BoolConstRegisterEnd - latte::Register::BoolConstRegisterBase) / 4 }, };

      mRingBuffer.writePM4(LoadLoopConst {
         phys_cast<phys_addr>(registers + (latte::Register::BoolConstRegisterBase / 4)),
         gsl::make_span(LoadBoolRange)
      });
   }

   void handleMemoryLoad(decaf::pm4::CaptureMemoryLoad &load, std::vector<char> &data)
   {
      std::memcpy(phys_cast<void *>(load.address).getRawPointer(),
                  data.data(), data.size());

      mGraphicsDriver->notifyCpuFlush(load.address,
                                      static_cast<uint32_t>(data.size()));
   }

   bool
   scanType0(HeaderType0 header,
             const gsl::span<be2_val<uint32_t>> &data)
   {
      return false;
   }

   bool
   scanType3(HeaderType3 header,
             const gsl::span<be2_val<uint32_t>> &data)
   {
      if (header.opcode() == IT_OPCODE::DECAF_SWAP_BUFFERS) {
         return true;
      }

      if (header.opcode() == IT_OPCODE::INDIRECT_BUFFER ||
          header.opcode() == IT_OPCODE::INDIRECT_BUFFER_PRIV) {
         return scanCommandBuffer(phys_cast<void *>(phys_addr { data[0].value() }).getRawPointer(),
                                  data[2]);
      }

      return false;
   }

   bool
   scanCommandBuffer(void *words, uint32_t numWords)
   {
      auto buffer = reinterpret_cast<be2_val<uint32_t> *>(words);
      auto foundSwap = false;

      for (auto pos = size_t { 0u }; pos < numWords; ) {
         auto header = Header::get(buffer[pos]);
         auto size = size_t { 0u };

         switch (header.type()) {
         case PacketType::Type0:
         {
            auto header0 = HeaderType0::get(header.value);
            size = header0.count() + 1;

            decaf_check(pos + size < numWords);
            foundSwap |= scanType0(header0, gsl::make_span(buffer + pos + 1, size));
            break;
         }
         case PacketType::Type3:
         {
            auto header3 = HeaderType3::get(header.value);
            size = header3.size() + 1;

            decaf_check(pos + size < numWords);
            foundSwap |= scanType3(header3, gsl::make_span(buffer + pos + 1, size));
            break;
         }
         case PacketType::Type2:
         {
            // This is a filler packet, like a "nop", ignore it
            break;
         }
         case PacketType::Type1:
         default:
            size = numWords;
            break;
         }

         pos += size + 1;
      }

      return foundSwap;
   }

private:
   gpu::GraphicsDriver *mGraphicsDriver = nullptr;
   RingBuffer mRingBuffer;
   std::ifstream mFile;
   phys_ptr<uint32_t> mRegisterStorage = nullptr;
};

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
SDLWindow::initGlGraphics()
{
#ifdef DECAF_GL
   mRenderer = new DecafSDLOpenGL();
   if (!mRenderer->initialise(WindowWidth, WindowHeight, false)) {
      gCliLog->error("Failed to create GL graphics window");
      return false;
   }

   mRendererName = "GL";
   return true;
#else
   decaf_abort("GL support was not included in this build");
#endif
}

bool
SDLWindow::initVulkanGraphics()
{
#ifdef DECAF_VULKAN
   mRenderer = new DecafSDLVulkan();
   if (!mRenderer->initialise(WindowWidth, WindowHeight, false)) {
      gCliLog->error("Failed to create Vulkan graphics window");
      return false;
   }

   mRendererName = "Vulkan";
   return true;
#else
   decaf_abort("Vulkan support was not included in this build");
#endif
}

void
SDLWindow::calculateScreenViewports(Viewport &tv, Viewport &drc)
{
   int TvWidth = 1280;
   int TvHeight = 720;

   int DrcWidth = 854;
   int DrcHeight = 480;

   int OuterBorder = 0;
   int ScreenSeparation = 5;

   int windowWidth, windowHeight;
   int nativeHeight, nativeWidth;
   int tvLeft, tvBottom, tvTop, tvRight;
   int drcLeft, drcBottom, drcTop, drcRight;

   auto tvVisible = true;
   auto drcVisible = true;

   mRenderer->getWindowSize(&windowWidth, &windowHeight);

   if (config::display::layout == config::display::Toggle) {
      // For toggle mode only one screen is visible at a time, so we calculate the
      // screen position as if only the TV exists here
      nativeHeight = TvHeight;
      nativeWidth = TvWidth;
      DrcWidth = 0;
      DrcHeight = 0;
      ScreenSeparation = 0;
   } else {
      nativeHeight = DrcHeight + TvHeight + ScreenSeparation + 2 * OuterBorder;
      nativeWidth = std::max(DrcWidth, TvWidth) + 2 * OuterBorder;
   }

   if (windowWidth * nativeHeight >= windowHeight * nativeWidth) {
      // Align to height
      int drcBorder = (windowWidth * nativeHeight - windowHeight * DrcWidth + nativeHeight) / nativeHeight / 2;
      int tvBorder = config::display::stretch ? 0 : (windowWidth * nativeHeight - windowHeight * TvWidth + nativeHeight) / nativeHeight / 2;

      drcBottom = OuterBorder;
      drcTop = OuterBorder + (DrcHeight * windowHeight + nativeHeight / 2) / nativeHeight;
      drcLeft = drcBorder;
      drcRight = windowWidth - drcBorder;

      tvBottom = windowHeight - OuterBorder - (TvHeight * windowHeight + nativeHeight / 2) / nativeHeight;
      tvTop = windowHeight - OuterBorder;
      tvLeft = tvBorder;
      tvRight = windowWidth - tvBorder;
   } else {
      // Align to width
      int heightBorder = (windowHeight * nativeWidth - windowWidth * (DrcHeight + TvHeight + ScreenSeparation) + nativeWidth) / nativeWidth / 2;
      int drcBorder = (windowWidth - DrcWidth * windowWidth / nativeWidth + 1) / 2;
      int tvBorder = (windowWidth - TvWidth * windowWidth / nativeWidth + 1) / 2;

      drcBottom = heightBorder;
      drcTop = heightBorder + (DrcHeight * windowWidth + nativeWidth / 2) / nativeWidth;
      drcLeft = drcBorder;
      drcRight = windowWidth - drcBorder;

      tvTop = windowHeight - heightBorder;
      tvBottom = windowHeight - heightBorder - (TvHeight * windowWidth + nativeWidth / 2) / nativeWidth;
      tvLeft = tvBorder;
      tvRight = windowWidth - tvBorder;
   }

   if (config::display::layout == config::display::Toggle) {
      // In toggle mode, DRC and TV size are the same
      drcLeft = tvLeft;
      drcRight = tvRight;
      drcTop = tvTop;
      drcBottom = tvBottom;

      if (mToggleDRC) {
         drcVisible = true;
         tvVisible = false;
      } else {
         drcVisible = false;
         tvVisible = true;
      }
   }

   if (drcVisible) {
      drc.x = static_cast<float>(drcLeft);
      drc.y = static_cast<float>(drcBottom);
      drc.width = static_cast<float>(drcRight - drcLeft);
      drc.height = static_cast<float>(drcTop - drcBottom);
   } else {
      drc.x = 0.0f;
      drc.y = 0.0f;
      drc.width = 0.0f;
      drc.height = 0.0f;
   }

   if (tvVisible) {
      tv.x = static_cast<float>(tvLeft);
      tv.y = static_cast<float>(tvBottom);
      tv.width = static_cast<float>(tvRight - tvLeft);
      tv.height = static_cast<float>(tvTop - tvBottom);
   } else {
      tv.x = 0.0f;
      tv.y = 0.0f;
      tv.width = 0.0f;
      tv.height = 0.0f;
   }

   decaf_check(drc.x >= 0);
   decaf_check(drc.y >= 0);
   decaf_check(drc.x + drc.width <= windowWidth);
   decaf_check(drc.y + drc.height <= windowHeight);
   decaf_check(tv.x >= 0);
   decaf_check(tv.y >= 0);
   decaf_check(tv.x + tv.width <= windowWidth);
   decaf_check(tv.y + tv.height <= windowHeight);
}

bool
SDLWindow::run(const std::string &tracePath)
{
   auto shouldQuit = false;

   // Setup replay heap
   sReplayHeap = phys_cast<cafe::TinyHeapPhysical *>(phys_addr { 0x34000000 });
   cafe::TinyHeap_Setup(sReplayHeap,
                        0x430,
                        phys_cast<void *>(phys_addr { 0x34000000 + 0x430 }),
                        0x1C000000 - 0x430);

   // Setup graphics driver
   auto graphicsDriver = mRenderer->getDecafDriver();
   decaf::setGraphicsDriver(graphicsDriver);

   // Run the loop!
   PM4Parser parser { graphicsDriver };

   if (!parser.open(tracePath)) {
      return false;
   }

   // Set swap interval to 1 otherwise frames will render super fast!
   SDL_GL_SetSwapInterval(1);

   while (!shouldQuit && !decaf::hasExited()) {
      SDL_Event event;

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

      if (parser.eof() || !parser.readFrame()) {
         shouldQuit = true;
         break;
      }

      Viewport tvViewport, drcViewport;
      calculateScreenViewports(tvViewport, drcViewport);
      mRenderer->renderFrame(tvViewport, drcViewport);
   }

   return true;
}
