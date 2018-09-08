#pragma optimize("",off)
#include "sdl_window.h"
#include "clilog.h"
#include "config.h"

#include <array>
#include <fstream>
#include <common/log.h>
#include <common/platform_dir.h>
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

static phys_ptr<cafe::TinyHeapPhysical>
sReplayHeap = nullptr;

phys_ptr<void>
sCommandBufferPool = nullptr;

constexpr uint32_t
sCommandBufferSize = 0x2000;

constexpr uint32_t
CommandBufferSize = 0x100;

struct ActiveCommandBuffer
{
   phys_ptr<uint32_t> buffer;
   uint32_t numWords;
};

ActiveCommandBuffer *
sActiveCommandBuffer = nullptr;

using namespace latte::pm4;

static void
flushCommandBuffer();

static ActiveCommandBuffer *
getCommandBuffer(uint32_t size)
{
   if (sActiveCommandBuffer &&
       sActiveCommandBuffer->numWords + size >= CommandBufferSize) {
      flushCommandBuffer();
   }

   if (!sActiveCommandBuffer) {
      sActiveCommandBuffer = new ActiveCommandBuffer();
      sActiveCommandBuffer->numWords = 0;

      auto allocPtr = phys_ptr<void> { };
      cafe::TinyHeap_Alloc(sReplayHeap, CommandBufferSize * sizeof(uint32_t), 0x100, &allocPtr);
      sActiveCommandBuffer->buffer = phys_cast<uint32_t *>(allocPtr);
   }

   return sActiveCommandBuffer;
}

static void
flushCommandBuffer()
{
   gpu::ringbuffer::submit(sActiveCommandBuffer,
                           sActiveCommandBuffer->buffer,
                           sActiveCommandBuffer->numWords);
   sActiveCommandBuffer = nullptr;
}

template<typename Type>
static void
writePM4(const Type &value)
{
   auto &ncValue = const_cast<Type &>(value);

   // Calculate the total size this object will be
   latte::pm4::PacketSizer sizer;
   ncValue.serialise(sizer);
   auto totalSize = sizer.getSize() + 1;

   // Serialize the packet to the active command buffer
   auto buffer = getCommandBuffer(totalSize);
   auto writer = latte::pm4::PacketWriter {
      buffer->buffer.getRawPointer(),
      buffer->numWords,
      Type::Opcode,
      totalSize
   };
   ncValue.serialise(writer);
}

static void
onRetireCallback(void *context)
{
   auto cb = reinterpret_cast<ActiveCommandBuffer *>(context);
   cafe::TinyHeap_Free(sReplayHeap, cb->buffer);
   delete cb;
}

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
            mFile.read(reinterpret_cast<char *>(mRegisterStorage.getRawPointer()), packet.size);

            // Swap it into big endian, so we can write LOAD_ commands
            for (auto i = 0u; i < numRegisters; ++i) {
               mRegisterStorage[i] = byte_swap(mRegisterStorage[i]);
            }

            handleRegisterSnapshot(mRegisterStorage, numRegisters);
            flushCommandBuffer();
            break;
         }
         case decaf::pm4::CapturePacket::SetBuffer:
         {
            decaf::pm4::CaptureSetBuffer setBuffer;
            mFile.read(reinterpret_cast<char *>(&setBuffer), sizeof(decaf::pm4::CaptureSetBuffer));

            handleSetBuffer(setBuffer);
            flushCommandBuffer();
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
   bool handleCommandBuffer(void *buffer, uint32_t sizeBytes)
   {
      auto allocPtr = phys_ptr<void> { nullptr };
      cafe::TinyHeap_Alloc(sReplayHeap, sizeBytes, 0x100, &allocPtr);

      auto ab = new ActiveCommandBuffer();
      ab->numWords = sizeBytes / 4u;
      ab->buffer = phys_cast<uint32_t *>(allocPtr);
      std::memcpy(ab->buffer.getRawPointer(),
                  buffer,
                  sizeBytes);

      gpu::ringbuffer::submit(ab, ab->buffer, ab->numWords);
      return scanCommandBuffer(buffer, ab->numWords);
   }

   void handleSetBuffer(decaf::pm4::CaptureSetBuffer &setBuffer)
   {
      auto isTv = (setBuffer.type == decaf::pm4::CaptureSetBuffer::TvBuffer) ? 1u : 0u;

      writePM4(DecafSetBuffer {
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

      writePM4(ContextControl {
         LOAD_CONTROL,
         SHADOW_ENABLE
      });

      // Write all the register load packets!
      static std::pair<uint32_t, uint32_t>
      LoadConfigRange[] = { { 0, (latte::Register::ConfigRegisterEnd - latte::Register::ConfigRegisterBase) / 4 }, };

      writePM4(LoadConfigReg {
         phys_cast<phys_addr>(registers + (latte::Register::ConfigRegisterBase / 4)),
         gsl::make_span(LoadConfigRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadContextRange[] = { { 0, (latte::Register::ContextRegisterEnd - latte::Register::ContextRegisterBase) / 4 }, };

      writePM4(LoadContextReg {
         phys_cast<phys_addr>(registers + (latte::Register::ContextRegisterBase / 4)),
         gsl::make_span(LoadContextRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadAluConstRange[] = { { 0, (latte::Register::AluConstRegisterEnd - latte::Register::AluConstRegisterBase) / 4 }, };

      writePM4(LoadAluConst {
         phys_cast<phys_addr>(registers + (latte::Register::AluConstRegisterBase / 4)),
         gsl::make_span(LoadAluConstRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadResourceRange[] = { { 0, (latte::Register::ResourceRegisterEnd - latte::Register::ResourceRegisterBase) / 4 }, };

      writePM4(latte::pm4::LoadResource {
         phys_cast<phys_addr>(registers + (latte::Register::ResourceRegisterBase / 4)),
         gsl::make_span(LoadResourceRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadSamplerRange[] = { { 0, (latte::Register::SamplerRegisterEnd - latte::Register::SamplerRegisterBase) / 4 }, };

      writePM4(LoadSampler {
         phys_cast<phys_addr>(registers + (latte::Register::SamplerRegisterBase / 4)),
         gsl::make_span(LoadSamplerRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadControlRange[] = { { 0, (latte::Register::ControlRegisterEnd - latte::Register::ControlRegisterBase) / 4 }, };

      writePM4(LoadControlConst {
         phys_cast<phys_addr>(registers + (latte::Register::ControlRegisterBase / 4)),
         gsl::make_span(LoadControlRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadLoopRange[] = { { 0, (latte::Register::LoopConstRegisterEnd - latte::Register::LoopConstRegisterBase) / 4 }, };

      writePM4(LoadLoopConst {
         phys_cast<phys_addr>(registers + (latte::Register::LoopConstRegisterBase / 4)),
         gsl::make_span(LoadLoopRange)
      });

      static std::pair<uint32_t, uint32_t>
      LoadBoolRange[] = { { 0, (latte::Register::BoolConstRegisterEnd - latte::Register::BoolConstRegisterBase) / 4 }, };

      writePM4(LoadLoopConst {
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

      if (header.opcode() == IT_OPCODE::INDIRECT_BUFFER_PRIV) {
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
   std::ifstream mFile;
   std::vector<uint8_t *> mBuffers;
   phys_ptr<uint32_t> mRegisterStorage = nullptr;
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

   // Setup replay heap
   sReplayHeap = phys_cast<cafe::TinyHeapPhysical *>(phys_addr { 0x34000000 });
   cafe::TinyHeap_Setup(sReplayHeap,
                        0x430,
                        phys_cast<void *>(phys_addr { 0x34000000 + 0x430 }),
                        0x1C000000 - 0x430);

   // Setup pm4 command buffer pool
   auto allocPtr = phys_ptr<void> { nullptr };
   cafe::TinyHeap_Alloc(sReplayHeap,
                        sCommandBufferSize * 4,
                        0x100,
                        &allocPtr);
   sCommandBufferPool = allocPtr;

   // Setup GPU retire callback
   gpu::setRetireCallback(onRetireCallback);

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

      mGraphicsDriver->runUntilFlip();

      gl::GLuint tvBuffer = 0;
      gl::GLuint drcBuffer = 0;
      mGraphicsDriver->getSwapBuffers(&tvBuffer, &drcBuffer);

      SDL_GL_MakeCurrent(mWindow, mWindowContext);
      drawScanBuffers(tvBuffer, drcBuffer);
      SDL_GL_MakeCurrent(mWindow, mGpuContext);
   }

   return true;
}
