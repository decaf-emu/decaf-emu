#include "sdl_window.h"
#include "clilog.h"
#include "config.h"

#include <array>
#include <fstream>
#include <common/teenyheap.h>
#include <common/platform_dir.h>
#include <libcpu/mmu.h>
#include <libcpu/pointer.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <libdecaf/decaf_pm4replay.h>
#include <libdecaf/src/kernel/kernel_memory.h>
#include <libdecaf/src/modules/gx2/gx2_internal_cbpool.h>
#include <libdecaf/src/modules/gx2/gx2_state.h>
#include <libgpu/gpu_config.h>
#include <libgpu/latte/latte_registers.h>
#include <libgpu/latte/latte_pm4.h>
#include <libgpu/latte/latte_pm4_commands.h>
#include <libgpu/latte/latte_pm4_reader.h>
#include <libgpu/latte/latte_pm4_writer.h>

static TeenyHeap *
gSystemHeap = nullptr;

using namespace latte::pm4;

class PM4Parser
{
public:
   PM4Parser(gpu::GraphicsDriver *driver) :
      mGraphicsDriver(driver)
   {
      mRegisterStorage = reinterpret_cast<uint32_t *>(gSystemHeap->alloc(0x10000 * 4, 0x100));
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

      // Free command buffers used from last frame
      for (auto buf : mBuffers) {
         delete[] buf;
      }

      mBuffers.clear();

      while (!foundSwap) {
         decaf::pm4::CapturePacket packet;
         mFile.read(reinterpret_cast<char *>(&packet), sizeof(decaf::pm4::CapturePacket));

         if (!mFile) {
            return false;
         }

         switch (packet.type) {
         case decaf::pm4::CapturePacket::CommandBuffer:
         {
            auto commandBuffer = new uint8_t[packet.size];
            mFile.read(reinterpret_cast<char *>(commandBuffer), packet.size);

            if (!mFile) {
               return false;
            }

            foundSwap |= handleCommandBuffer(commandBuffer, packet.size);
            mBuffers.push_back(commandBuffer);
            break;
         }
         case decaf::pm4::CapturePacket::RegisterSnapshot:
         {
            decaf_check((packet.size % 4) == 0);
            auto numRegisters = packet.size / 4;
            mFile.read(reinterpret_cast<char *>(mRegisterStorage), packet.size);

            // Swap it into big endian, so we can write LOAD_ commands
            for (auto i = 0u; i < numRegisters; ++i) {
               mRegisterStorage[i] = byte_swap(mRegisterStorage[i]);
            }

            handleRegisterSnapshot(reinterpret_cast<be_val<uint32_t> *>(mRegisterStorage), numRegisters);
            gx2::internal::flushCommandBuffer(0x100);
            break;
         }
         case decaf::pm4::CapturePacket::SetBuffer:
         {
            decaf::pm4::CaptureSetBuffer setBuffer;
            mFile.read(reinterpret_cast<char *>(&setBuffer), sizeof(decaf::pm4::CaptureSetBuffer));

            handleSetBuffer(setBuffer);
            gx2::internal::flushCommandBuffer(0x100);
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

      return foundSwap;
   }

private:
   bool handleCommandBuffer(void *buffer, uint32_t size)
   {
      decaf::pm4::injectCommandBuffer(buffer, size);
      return scanCommandBuffer(buffer, size / 4);
   }

   void handleSetBuffer(decaf::pm4::CaptureSetBuffer &setBuffer)
   {
      auto isTv = (setBuffer.type == decaf::pm4::CaptureSetBuffer::TvBuffer) ? 1u : 0u;

      gx2::internal::writePM4(DecafSetBuffer {
         isTv,
         setBuffer.bufferingMode,
         setBuffer.width,
         setBuffer.height
      });
   }

   void handleRegisterSnapshot(be_val<uint32_t> *registers, uint32_t count)
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

      gx2::internal::writePM4(ContextControl {
         LOAD_CONTROL,
         SHADOW_ENABLE
      });

      // Write all the register load packets!
      static std::pair<uint32_t, uint32_t>
      LoadConfigRange[] = { { 0, (latte::Register::ConfigRegisterEnd - latte::Register::ConfigRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadConfigReg {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::ConfigRegisterBase / 4]),
         gsl::make_span(LoadConfigRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadContextRange[] = { { 0, (latte::Register::ContextRegisterEnd - latte::Register::ContextRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadContextReg {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::ContextRegisterBase / 4]),
         gsl::make_span(LoadContextRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadAluConstRange[] = { { 0, (latte::Register::AluConstRegisterEnd - latte::Register::AluConstRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadAluConst {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::AluConstRegisterBase / 4]),
         gsl::make_span(LoadAluConstRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadResourceRange[] = { { 0, (latte::Register::ResourceRegisterEnd - latte::Register::ResourceRegisterBase) / 4 }, };

      gx2::internal::writePM4(latte::pm4::LoadResource {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::ResourceRegisterBase / 4]),
         gsl::make_span(LoadResourceRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadSamplerRange[] = { { 0, (latte::Register::SamplerRegisterEnd - latte::Register::SamplerRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadSampler {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::SamplerRegisterBase / 4]),
         gsl::make_span(LoadSamplerRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadControlRange[] = { { 0, (latte::Register::ControlRegisterEnd - latte::Register::ControlRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadControlConst {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::ControlRegisterBase / 4]),
         gsl::make_span(LoadControlRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadLoopRange[] = { { 0, (latte::Register::LoopConstRegisterEnd - latte::Register::LoopConstRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadLoopConst {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::LoopConstRegisterBase / 4]),
         gsl::make_span(LoadLoopRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadBoolRange[] = { { 0, (latte::Register::BoolConstRegisterEnd - latte::Register::BoolConstRegisterBase) / 4 }, };

      gx2::internal::writePM4(LoadLoopConst {
         reinterpret_cast<be_val<uint32_t> *>(&registers[latte::Register::BoolConstRegisterBase / 4]),
         gsl::make_span(LoadBoolRange)
      });
   }

   void handleMemoryLoad(decaf::pm4::CaptureMemoryLoad &load, std::vector<char> &data)
   {
      auto ptr = mem::translate(load.address);
      std::memcpy(ptr, data.data(), data.size());
   }

   bool
   scanType0(HeaderType0 header,
             const gsl::span<be_val<uint32_t>> &data)
   {
      return false;
   }

   bool
   scanType3(HeaderType3 header,
             const gsl::span<be_val<uint32_t>> &data)
   {
      if (header.opcode() == IT_OPCODE::DECAF_SWAP_BUFFERS) {
         return true;
      }

      if (header.opcode() == IT_OPCODE::INDIRECT_BUFFER_PRIV) {
         return scanCommandBuffer(mem::translate(data[0]), data[2]);
      }

      return false;
   }

   bool
   scanCommandBuffer(void *words, uint32_t numWords)
   {
      auto buffer = reinterpret_cast<be_val<uint32_t> *>(words);
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
            foundSwap |= scanType0(header0, gsl::make_span(&buffer[pos + 1], size));
            break;
         }
         case PacketType::Type3:
         {
            auto header3 = HeaderType3::get(header.value);
            size = header3.size() + 1;

            decaf_check(pos + size < numWords);
            foundSwap |= scanType3(header3, gsl::make_span(&buffer[pos + 1], size));
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
   std::ifstream mFile;
   std::vector<uint8_t *> mBuffers;
   uint32_t *mRegisterStorage = nullptr;
};

SDLWindow::~SDLWindow()
{
   if (mWindowContext) {
      SDL_GL_DeleteContext(mWindowContext);
      mWindowContext = nullptr;
   }

   if (mGpuContext) {
      SDL_GL_DeleteContext(mGpuContext);
      mGpuContext = nullptr;
   }

   if (mWindow) {
      SDL_DestroyWindow(mWindow);
      mWindow = nullptr;
   }
}

bool
SDLWindow::createWindow()
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
      gCliLog->error("Failed to initialize SDL: {}", SDL_GetError());
      return false;
   }

   if (SDL_GL_LoadLibrary(NULL) != 0) {
      gCliLog->error("Failed to load OpenGL library: {}", SDL_GetError());
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

   // Set to OpenGL 4.5 core profile
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

   // Enable debug context
   if (gpu::config::debug) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
   }

   // Create TV window
   mWindow = SDL_CreateWindow("Decaf PM4 Replay",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WindowWidth, WindowHeight,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

   if (!mWindow) {
      gCliLog->error("Failed to create TV window: {}", SDL_GetError());
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

   // Create OpenGL context
   mWindowContext = SDL_GL_CreateContext(mWindow);

   if (!mWindowContext) {
      gCliLog->error("Failed to create Main OpenGL context: {}", SDL_GetError());
      return false;
   }

   mGpuContext = SDL_GL_CreateContext(mWindow);

   if (!mGpuContext) {
      gCliLog->error("Failed to create GPU OpenGL context: {}", SDL_GetError());
      return false;
   }

   SDL_GL_MakeCurrent(mWindow, mWindowContext);
   return true;
}

bool
SDLWindow::run(const std::string &tracePath)
{
   auto shouldQuit = false;

   // Setup OpenGL graphics driver
   auto glDriver = gpu::createGLDriver();
   mGraphicsDriver = reinterpret_cast<gpu::OpenGLDriver *>(glDriver);

   if (config::dump_drc_frames || config::dump_tv_frames) {
      platform::createDirectory(config::dump_frames_dir);
      mGraphicsDriver->startFrameCapture(config::dump_frames_dir + "/replay",
                                         config::dump_tv_frames,
                                         config::dump_drc_frames);
   }

   // Setup rendering
   SDL_GL_MakeCurrent(mWindow, mWindowContext);
   SDL_GL_SetSwapInterval(1);
   initialiseContext();
   initialiseDraw();

   SDL_GL_SetSwapInterval(1);
   SDL_GL_MakeCurrent(mWindow, mGpuContext);
   initialiseContext();

   // Setup decaf shit
   kernel::initialiseVirtualMemory();
   kernel::initialiseAppMemory(0x10000);
   auto systemHeapBounds = kernel::getSystemHeapBounds();
   gSystemHeap = new TeenyHeap { cpu::VirtualPointer<void> { systemHeapBounds.start }.getRawPointer(), systemHeapBounds.size };

   // Setup pm4 command buffer pool
   auto cbPoolSize = 0x2000;
   auto cbPoolBase = gSystemHeap->alloc(cbPoolSize, 0x100);

   gx2::internal::setMainCore();
   gx2::internal::initCommandBufferPool(reinterpret_cast<uint32_t *>(cbPoolBase), cbPoolSize / 4);

   // Run the loop!
   PM4Parser parser { mGraphicsDriver };

   if (!parser.open(tracePath)) {
      return false;
   }

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

      // Parser has read a frame, so lets spam syncPoll until the GPU has spat out a frame for us
      auto noDraw = true;

      while (noDraw) {
         mGraphicsDriver->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
            SDL_GL_MakeCurrent(mWindow, mWindowContext);
            drawScanBuffers(tvBuffer, drcBuffer);
            SDL_GL_MakeCurrent(mWindow, mGpuContext);
            noDraw = false;
         });
      }
   }

   return true;
}
