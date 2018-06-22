
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>

#include <common/log.h>
#include <common/teenyheap.h>
#include <libdecaf/decaf.h>
#include <libdecaf/src/kernel/kernel_memory.h>
#include <libdecaf/src/modules/gx2/gx2_internal_cbpool.h>
#include <libdecaf/src/modules/gx2/gx2_state.h>
#include <libcpu/cpu.h>
#include <libcpu/pointer.h>
#include <libcpu/be2_struct.h>
#include <libgpu/gpu_config.h>
#include <libgpu/gpu_opengldriver.h>
#include <spdlog/spdlog.h>

#include <condition_variable>
#include <queue>

#include "replay.h"
#include "replayrunner.h"

using namespace latte::pm4;
static std::string
getGlDebugSource(gl::GLenum source)
{
   switch (source) {
   case GL_DEBUG_SOURCE_API:
      return "API";
   case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
      return "WINSYS";
   case GL_DEBUG_SOURCE_SHADER_COMPILER:
      return "COMPILER";
   case GL_DEBUG_SOURCE_THIRD_PARTY:
      return "EXTERNAL";
   case GL_DEBUG_SOURCE_APPLICATION:
      return "APP";
   case GL_DEBUG_SOURCE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(source);
   }
}

static std::string
getGlDebugType(gl::GLenum severity)
{
   switch (severity) {
   case GL_DEBUG_TYPE_ERROR:
      return "ERROR";
   case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "DEPRECATED_BEHAVIOR";
   case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UNDEFINED_BEHAVIOR";
   case GL_DEBUG_TYPE_PORTABILITY:
      return "PORTABILITY";
   case GL_DEBUG_TYPE_PERFORMANCE:
      return "PERFORMANCE";
   case GL_DEBUG_TYPE_MARKER:
      return "MARKER";
   case GL_DEBUG_TYPE_PUSH_GROUP:
      return "PUSH_GROUP";
   case GL_DEBUG_TYPE_POP_GROUP:
      return "POP_GROUP";
   case GL_DEBUG_TYPE_OTHER:
      return "OTHER";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static std::string
getGlDebugSeverity(gl::GLenum severity)
{
   switch (severity) {
   case GL_DEBUG_SEVERITY_HIGH:
      return "HIGH";
   case GL_DEBUG_SEVERITY_MEDIUM:
      return "MED";
   case GL_DEBUG_SEVERITY_LOW:
      return "LOW";
   case GL_DEBUG_SEVERITY_NOTIFICATION:
      return "NOTIF";
   default:
      return glbinding::Meta::getString(severity);
   }
}

static void GL_APIENTRY
debugMessageCallback(gl::GLenum source, gl::GLenum type, gl::GLuint id, gl::GLenum severity,
                     gl::GLsizei length, const gl::GLchar* message, const void *userParam)
{
   for (auto filterID : gpu::config::debug_filters) {
      if (filterID == id) {
         return;
      }
   }

   auto outputStr = fmt::format("GL Message ({}, {}, {}, {}) {}", id,
                                getGlDebugSource(source),
                                getGlDebugType(type),
                                getGlDebugSeverity(severity),
                                message);

   if (severity == GL_DEBUG_SEVERITY_HIGH) {
      gLog->warn(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
      gLog->debug(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_LOW) {
      gLog->trace(outputStr);
   } else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
      gLog->info(outputStr);
   } else {
      gLog->info(outputStr);
   }
}

void ReplayRunner::initialise()
{
   mDecaf->context()->makeCurrent(mDecaf->surface());
   glbinding::Binding::initialize();

   if (gpu::config::debug) {
      glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
      glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
         auto error = glbinding::Binding::GetError.directCall();

         if (error != GL_NO_ERROR) {
            fmt::MemoryWriter writer;
            writer << call.function->name() << "(";

            for (unsigned i = 0; i < call.parameters.size(); ++i) {
               writer << call.parameters[i]->asString();
               if (i < call.parameters.size() - 1)
                  writer << ", ";
            }

            writer << ")";

            if (call.returnValue) {
               writer << " -> " << call.returnValue->asString();
            }

            gLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
         }
      });

      gl::glDebugMessageCallback(&debugMessageCallback, nullptr);
      gl::glEnable(static_cast<gl::GLenum>(GL_DEBUG_OUTPUT));
      gl::glEnable(static_cast<gl::GLenum>(GL_DEBUG_OUTPUT_SYNCHRONOUS));
   }
}

void ReplayRunner::runGpu()
{
   mDecaf->context()->makeCurrent(mDecaf->surface());
   mDecaf->graphicsDriver()->syncPoll([&](unsigned int tvBuffer, unsigned int drcBuffer) {
      emit frameFinished(tvBuffer, drcBuffer);
   });
}

void ReplayRunner::runFrame()
{
   auto foundFrameTerminator = false;

   while (mRunning && mPosition.packetIndex < mReplay->index.packets.size()) {
      foundFrameTerminator = runPacket(mReplay->index.packets[mPosition.packetIndex]);

      if (foundFrameTerminator) {
         break;
      }

      mPosition.packetIndex++;
   }

   runGpu();

   if (!foundFrameTerminator) {
      emit replayFinished();
   }
}

bool ReplayRunner::runPacket(ReplayIndex::Packet &packet)
{
   auto foundFrameTerminator = false;

   switch (packet.type) {
   case decaf::pm4::CapturePacket::CommandBuffer:
   {
      auto commandIndex = 0;
      auto packetEnd = packet.data + packet.size;

      while (mRunning && !foundFrameTerminator) {
         auto &command = mReplay->index.commands[mPosition.commandIndex];

         if (command.command >= packetEnd) {
            break;
         }

         foundFrameTerminator = runCommand(command);
         mPosition.commandIndex++;
      }

      gx2::internal::flushCommandBuffer(0x100);
      break;
   }
   case decaf::pm4::CapturePacket::MemoryLoad:
   {
      auto loadPacket = reinterpret_cast<decaf::pm4::CaptureMemoryLoad *>(packet.data);
      auto loadData = packet.data + sizeof(decaf::pm4::CaptureMemoryLoad);
      auto dst = virt_cast<void *>(static_cast<virt_addr>(loadPacket->address));
      std::memcpy(dst.getRawPointer(), loadData, packet.size - sizeof(decaf::pm4::CaptureMemoryLoad));
      break;
   }
   case decaf::pm4::CapturePacket::RegisterSnapshot:
   {
      auto registers = reinterpret_cast<uint32_t *>(packet.data);
      auto count = packet.size / sizeof(uint32_t);

      for (auto i = 0u; i < count; ++i) {
         mRegisterStorage[i] = registers[i];
      }

      runRegisterSnapshot(mRegisterStorage, count);
      gx2::internal::flushCommandBuffer(0x100);
      break;
   }
   case decaf::pm4::CapturePacket::SetBuffer:
   {
      auto setBufferPacket = reinterpret_cast<decaf::pm4::CaptureSetBuffer *>(packet.data);
      auto isTv = (setBufferPacket->type == decaf::pm4::CaptureSetBuffer::TvBuffer) ? 1u : 0u;

      gx2::internal::writePM4(DecafSetBuffer {
         isTv,
         setBufferPacket->bufferingMode,
         setBufferPacket->width,
         setBufferPacket->height
      });
      gx2::internal::flushCommandBuffer(0x100);
      break;
   }
   }

   return foundFrameTerminator;
}

void ReplayRunner::runRegisterSnapshot(be_val<uint32_t> *registers, uint32_t count)
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

bool ReplayRunner::runCommand(ReplayIndex::Command &command)
{
   auto foundFrameTerminator = false;

   switch (command.header.type()) {
   case PacketType::Type3:
   {
      auto header3 = HeaderType3::get(command.header.value);
      auto size = header3.size() + 1;

      if (header3.opcode() == IT_OPCODE::DECAF_SWAP_BUFFERS) {
         foundFrameTerminator = true;
      }

      if (header3.opcode() == IT_OPCODE::INDIRECT_BUFFER_PRIV) {
         // Should we iterate through commands in an indirect buffer??
      }

      decaf::pm4::injectCommandBuffer(command.command, (size + 1) * 4);
      break;
   }
   case PacketType::Type0:
   {
      auto header0 = HeaderType0::get(command.header.value);
      auto size = header0.count() + 1;
      decaf::pm4::injectCommandBuffer(command.command, (size + 1) * 4);
      break;
   }
   default:
      // FUCK
      break;
   }

   return foundFrameTerminator;
}
