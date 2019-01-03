#ifdef DECAF_GL
#include "gpu_config.h"
#include "gpu_memory.h"
#include "latte/latte_registers.h"
#include "opengl_constants.h"
#include "opengl_driver.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>
#include <glbinding/gl/gl.h>
#include <libcpu/mem.h>

namespace opengl
{

bool
GLDriver::checkActiveFeedbackBuffers()
{
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);
   auto vgt_strmout_buffer_en = getRegister<latte::VGT_STRMOUT_BUFFER_EN>(latte::Register::VGT_STRMOUT_BUFFER_EN);

   if (!vgt_strmout_en.STREAMOUT()) {
      return true;
   }

   for (auto index = 0u; index < latte::MaxStreamOutBuffers; ++index) {
      if (!(vgt_strmout_buffer_en.value & (1 << index))) {
         if (mFeedbackBufferState[index].bound) {
            endTransformFeedback();
            gl::glBindBufferBase(gl::GL_TRANSFORM_FEEDBACK_BUFFER, index, 0);
            mFeedbackBufferState[index].bound = false;
         }
         continue;
      }

      auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * index);
      auto addr = vgt_strmout_buffer_base << 8;
      auto &buffer = mDataBuffers[addr];

      if (!buffer.object || !buffer.isOutput) {
         gLog->error("Attempt to use unconfigured feedback buffer {}", index);
         return false;
      }

      if (mFeedbackBufferState[index].bound && mFeedbackBufferState[index].object == buffer.object) {
         // Don't try to rebind the buffer if there's no change in buffer state
         //  since we could be in the middle of a paused transform feedback.
         continue;
      }

      auto offset = mFeedbackBufferState[index].baseOffset;

      if (offset >= buffer.allocatedSize) {
         gLog->error("Attempt to bind feedback buffer {} at offset {} >= size {}", index, offset, buffer.allocatedSize);
         return false;
      }

      endTransformFeedback();
      gl::glBindBufferRange(gl::GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer.object, offset, buffer.allocatedSize - offset);

      mFeedbackBufferState[index].bound = true;
      mFeedbackBufferState[index].object = buffer.object;
      mFeedbackBufferState[index].currentOffset = offset;
   }

   return true;
}

void
GLDriver::beginTransformFeedback(gl::GLenum primitive)
{
   decaf_check(!mFeedbackActive);

   // Must be a base primitive type (not strip/fan/etc.)
   decaf_check(primitive == gl::GL_POINTS || primitive == gl::GL_LINES || primitive == gl::GL_TRIANGLES);

   // Start a transform feedback counter query so we can properly return the
   //  buffer offsets when requested
   if (!mFeedbackQuery) {
      gl::glCreateQueries(gl::GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1, &mFeedbackQuery);
      decaf_check(mFeedbackQuery);

      if (mDebug) {
         gl::glObjectLabel(gl::GL_QUERY, mFeedbackQuery, -1, "transform feedback query");
      }
   }
   gl::glBeginQuery(gl::GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, mFeedbackQuery);

   gl::glBeginTransformFeedback(primitive);
   mFeedbackActive = true;
   mFeedbackPrimitive = primitive;
}

void
GLDriver::endTransformFeedback()
{
   if (!mFeedbackActive) {
      return;
   }

   gl::glEndTransformFeedback();
   mFeedbackActive = false;

   gl::glEndQuery(gl::GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

   auto vgt_strmout_buffer_en = getRegister<latte::VGT_STRMOUT_BUFFER_EN>(latte::Register::VGT_STRMOUT_BUFFER_EN);
   mFeedbackQueryBuffers = vgt_strmout_buffer_en.value & 0xF;

   for (auto i = 0u; i < 4; ++i) {
      auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
      mFeedbackQueryStride[i] = vgt_strmout_vtx_stride * 4;
   }
}

void
GLDriver::updateTransformFeedbackOffsets()
{
   if (mFeedbackQueryBuffers == 0) {
      return;
   }

   // The OpenGL specification and API reference differ on whether the
   //  TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query returns a primitive count
   //  or a vertex count, but actual experiments (using an NVIDIA GPU)
   //  indicate that it's a primitive count, so we treat it as such.
   //  We explicitly sleep until the query is ready because some drivers are
   //  bad at waiting for queries on their own and introduce extra delay.
   gl::GLint done = false;
   while (gl::glGetQueryObjectiv(mFeedbackQuery, gl::GL_QUERY_RESULT_AVAILABLE, &done), !done) {
      std::this_thread::sleep_for(std::chrono::microseconds(10));
   }
   gl::GLuint numPrimitives = 0;
   glGetQueryObjectuiv(mFeedbackQuery, gl::GL_QUERY_RESULT, &numPrimitives);

   int numVertices;
   switch (mFeedbackPrimitive) {
   case gl::GL_POINTS:
      numVertices = numPrimitives;
      break;
   case gl::GL_LINES:
      numVertices = numPrimitives * 2;
      break;
   case gl::GL_TRIANGLES:
      numVertices = numPrimitives * 3;
      break;
   default:
      decaf_abort(fmt::format("Impossible mFeedbackPrimitive 0x{:04X}", static_cast<int>(mFeedbackPrimitive)));
   }

   for (auto i = 0u; i < 4; ++i) {
      if (mFeedbackQueryBuffers & (1 << i)) {
         mFeedbackBufferState[i].currentOffset += numVertices * mFeedbackQueryStride[i];
      }
   }

   mFeedbackQueryBuffers = 0;
}

void
GLDriver::streamOutBaseUpdate(const latte::pm4::StreamOutBaseUpdate &data)
{
   // Nothing to do for now
}

void
GLDriver::streamOutBufferUpdate(const latte::pm4::StreamOutBufferUpdate &data)
{
   auto bufferIndex = data.control.SELECT_BUFFER();

   if (data.control.STORE_BUFFER_FILLED_SIZE()) {
      auto addr = data.dstLo;
      decaf_assert(data.dstHi == 0, fmt::format("Store target out of 32-bit range for feedback buffer {}", bufferIndex));

      if (addr) {
         // End any in-progress transform feedback so we can get the buffer
         //  offset to store
         endTransformFeedback();
         updateTransformFeedbackOffsets();

         auto offsetPtr = gpu::internal::translateAddress<uint32_t>(addr);
         *offsetPtr = byte_swap(mFeedbackBufferState[bufferIndex].currentOffset >> 2);
      }
   }

   auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * bufferIndex);
   auto vgt_strmout_buffer_size = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_SIZE_0 + 16 * bufferIndex);

   auto addr = phys_addr { vgt_strmout_buffer_base << 8 };
   auto size = vgt_strmout_buffer_size << 2;
   decaf_assert(addr && size, fmt::format("Attempt to bind undefined feedback buffer {}", bufferIndex));

   // Create the data buffer (we don't need to manipulate it here, but
   //  make sure it's configured as an output buffer)
   getDataBuffer(addr, size, false, true);

   switch (data.control.OFFSET_SOURCE()) {
   case latte::pm4::STRMOUT_OFFSET_FROM_PACKET:
      decaf_assert(data.srcHi == 0, fmt::format("Offset out of 32-bit range for feedback buffer {}", bufferIndex));
      mFeedbackBufferState[bufferIndex].baseOffset = data.srcLo << 2;
      mFeedbackBufferState[bufferIndex].bound = false;  // Force rebind on next draw
      break;

   case latte::pm4::STRMOUT_OFFSET_FROM_VGT_FILLED_SIZE:
      mFeedbackBufferState[bufferIndex].baseOffset = mFeedbackBufferState[bufferIndex].currentOffset;
      mFeedbackBufferState[bufferIndex].bound = false;
      break;

   case latte::pm4::STRMOUT_OFFSET_FROM_MEM:
   {
      decaf_assert(data.srcHi == 0, fmt::format("Load target out of 32-bit range for feedback buffer {}", bufferIndex));
      auto offsetPtr = gpu::internal::translateAddress<uint32_t>(data.srcLo);
      decaf_assert(offsetPtr, fmt::format("Invalid load address for feedback buffer {}", bufferIndex));
      mFeedbackBufferState[bufferIndex].baseOffset = byte_swap(*offsetPtr) << 2;
      mFeedbackBufferState[bufferIndex].bound = false;
      break;
   }

   case latte::pm4::STRMOUT_OFFSET_NONE:
      break;  // No change.
   }
}

} // namespace opengl

#endif // ifdef DECAF_GL
