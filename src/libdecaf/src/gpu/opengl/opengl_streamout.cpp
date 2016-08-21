#include "common/decaf_assert.h"
#include "common/log.h"
#include "decaf_config.h"
#include "gpu/latte_registers.h"
#include "opengl_constants.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

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

   // Start a transform feedback counter query so we can properly return the
   //  buffer offsets when requested
   if (!mFeedbackQuery) {
      gl::glCreateQueries(gl::GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, 1, &mFeedbackQuery);
      decaf_check(mFeedbackQuery);

      if (decaf::config::gpu::debug) {
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

   // TODO: Does the TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN query return a
   //  primitive count or a vertex count?  The name and the GL spec suggest
   //  the former, but the API reference explicitly states the latter
   //  ("increment the counter once for every _vertex_").  Assume for now
   //  that the API reference is correct on account of its being more explicit.
   gl::GLuint numVertices = 0;
   glGetQueryObjectuiv(mFeedbackQuery, gl::GL_QUERY_RESULT, &numVertices);

   for (auto i = 0u; i < 4; ++i) {
      if (vgt_strmout_buffer_en.value & (1 << i)) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
         auto stride = vgt_strmout_vtx_stride * 4;
         mFeedbackBufferState[i].currentOffset += numVertices * stride;
      }
   }
}

void
GLDriver::streamOutBaseUpdate(const pm4::StreamOutBaseUpdate &data)
{
   // Nothing to do for now
}

void
GLDriver::streamOutBufferUpdate(const pm4::StreamOutBufferUpdate &data)
{
   auto bufferIndex = data.control.SELECT_BUFFER();

   if (data.control.STORE_BUFFER_FILLED_SIZE()) {
      auto addr = data.dstLo;
      decaf_assert(data.dstHi == 0, fmt::format("Store target out of 32-bit range for feedback buffer {}", bufferIndex));

      if (addr != 0) {
         // End any in-progress transform feedback so we can get the buffer
         //  offset to store
         endTransformFeedback();

         auto offsetPtr = mem::translate<uint32_t>(addr);
         *offsetPtr = byte_swap(mFeedbackBufferState[bufferIndex].currentOffset >> 2);
      }
   }

   auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * bufferIndex);
   auto vgt_strmout_buffer_size = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_SIZE_0 + 16 * bufferIndex);

   auto addr = vgt_strmout_buffer_base << 8;
   auto size = vgt_strmout_buffer_size << 2;
   decaf_assert(addr != 0 && size != 0, fmt::format("Attempt to bind undefined feedback buffer {}", bufferIndex));

   // Create the data buffer (we don't need to manipulate it here, but
   //  make sure it's configured as an output buffer)
   getDataBuffer(addr, size, false, true);

   switch (data.control.OFFSET_SOURCE()) {
   case pm4::STRMOUT_OFFSET_FROM_PACKET:
      decaf_assert(data.srcHi == 0, fmt::format("Offset out of 32-bit range for feedback buffer {}", bufferIndex));
      mFeedbackBufferState[bufferIndex].baseOffset = data.srcLo << 2;
      mFeedbackBufferState[bufferIndex].bound = false;  // Force rebind on next draw
      break;

   case pm4::STRMOUT_OFFSET_FROM_VGT_FILLED_SIZE:
      mFeedbackBufferState[bufferIndex].baseOffset = mFeedbackBufferState[bufferIndex].currentOffset;
      mFeedbackBufferState[bufferIndex].bound = false;
      break;

   case pm4::STRMOUT_OFFSET_FROM_MEM:
   {
      decaf_assert(data.srcHi == 0, fmt::format("Load target out of 32-bit range for feedback buffer {}", bufferIndex));
      auto offsetPtr = mem::translate<uint32_t>(data.srcLo);
      decaf_assert(offsetPtr, fmt::format("Invalid load address for feedback buffer {}", bufferIndex));
      mFeedbackBufferState[bufferIndex].baseOffset = byte_swap(*offsetPtr) << 2;
      mFeedbackBufferState[bufferIndex].bound = false;
      break;
   }

   case pm4::STRMOUT_OFFSET_NONE:
      break;  // No change.
   }
}

} // namespace opengl

} // namespace gpu
