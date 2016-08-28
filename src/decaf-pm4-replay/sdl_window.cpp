#include "sdl_window.h"
#include "clilog.h"
#include <array>
#include <fstream>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <libdecaf/decaf_pm4replay.h>
#include <libdecaf/src/gpu/pm4_format.h>
#include <libdecaf/src/gpu/pm4_packets.h>
#include <libdecaf/src/gpu/pm4_reader.h>
#include <libcpu/mem.h>

class PM4Parser
{
public:
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
            mFile.seekg(std::ifstream::cur, packet.size);
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

   void handleMemoryLoad(decaf::pm4::CaptureMemoryLoad &load, std::vector<char> &data)
   {
      auto ptr = mem::translate(load.address);
      std::memcpy(ptr, data.data(), data.size());
   }

   bool
   scanType0(pm4::type0::Header header,
             const gsl::span<be_val<uint32_t>> &data)
   {
      return false;
   }

   bool
   scanType3(pm4::type3::Header header,
             const gsl::span<be_val<uint32_t>> &data)
   {
      if (header.opcode() == pm4::type3::DECAF_SWAP_BUFFERS) {
         return true;
      }

      if (header.opcode() == pm4::type3::INDIRECT_BUFFER_PRIV) {
         return scanCommandBuffer(mem::translate(data[0]), data[2]);
      }

      return false;
   }

   bool
   scanCommandBuffer(void *words, uint32_t numWords)
   {
      std::vector<uint32_t> swapped;
      auto buffer = reinterpret_cast<be_val<uint32_t> *>(words);
      auto foundSwap = false;

      for (auto pos = size_t { 0u }; pos < numWords; ) {
         auto header = pm4::Header::get(buffer[pos]);
         auto size = size_t { 0u };

         switch (header.type()) {
         case pm4::Header::Type0:
         {
            auto header0 = pm4::type0::Header::get(header.value);
            size = header0.count() + 1;

            decaf_check(pos + size < numWords);
            foundSwap |= scanType0(header0, gsl::as_span(&buffer[pos + 1], size));
            break;
         }
         case pm4::Header::Type3:
         {
            auto header3 = pm4::type3::Header::get(header.value);
            size = header3.size() + 1;

            decaf_check(pos + size < numWords);
            foundSwap |= scanType3(header3, gsl::as_span(&buffer[pos + 1], size));
            break;
         }
         case pm4::Header::Type2:
         {
            // This is a filler packet, like a "nop", ignore it
            break;
         }
         case pm4::Header::Type1:
         default:
            size = numWords;
            break;
         }

         pos += size + 1;
      }

      return foundSwap;
   }

private:
   std::ifstream mFile;
   std::vector<uint8_t *> mBuffers;
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
   if (decaf::config::gpu::debug) {
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
   mGraphicsDriver = decaf::createGLDriver();

   // Setup rendering
   SDL_GL_MakeCurrent(mWindow, mWindowContext);
   SDL_GL_SetSwapInterval(1);
   initialiseContext();
   initialiseDraw();

   SDL_GL_SetSwapInterval(1);
   SDL_GL_MakeCurrent(mWindow, mGpuContext);
   initialiseContext();

   // Setup decaf shit
   mem::initialise();

   // Run the loop!
   PM4Parser parser;

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
      }

      mGraphicsDriver->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
         SDL_GL_MakeCurrent(mWindow, mWindowContext);
         drawScanBuffers(tvBuffer, drcBuffer);
         SDL_GL_MakeCurrent(mWindow, mGpuContext);
      });
   }

   return true;
}
