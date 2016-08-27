#include "common/decaf_assert.h"
#include "decaf_config.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>
#include <glbinding/Meta.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkReadyDraw()
{
   if (!checkActiveShader()) {
      gLog->warn("Skipping draw with invalid shader.");
      return false;
   }

   if (!checkActiveAttribBuffers()) {
      gLog->warn("Skipping draw with invalid attribs.");
      return false;
   }

   if (!checkActiveUniforms()) {
      gLog->warn("Skipping draw with invalid uniforms.");
      return false;
   }

   if (!checkActiveFeedbackBuffers()) {
      gLog->warn("Skipping draw with invalid feedback buffers.");
      return false;
   }

   if (!checkActiveTextures()) {
      gLog->warn("Skipping draw with invalid textures.");
      return false;
   }

   if (!checkActiveSamplers()) {
      gLog->warn("Skipping draw with invalid samplers.");
      return false;
   }

   if (!checkActiveColorBuffer()) {
      gLog->warn("Skipping draw with invalid color buffer.");
      return false;
   }

   if (!checkActiveDepthBuffer()) {
      gLog->warn("Skipping draw with invalid depth buffer.");
      return false;
   }

   if (!checkAttribBuffersBound()) {
      static bool hasWarned = false;
      if (!hasWarned) {
         gLog->warn("The application is performing draws with unbound attribute buffers.");
         hasWarned = true;
      }
      return false;
   }

   if (mFramebufferChanged) {
      auto fbStatus = gl::glCheckFramebufferStatus(gl::GL_FRAMEBUFFER);

      if (fbStatus != gl::GL_FRAMEBUFFER_COMPLETE) {
         gLog->warn("Draw attempted with an incomplete framebuffer, status {}.", glbinding::Meta::getString(fbStatus));
         return false;
      }

      mFramebufferChanged = false;
   }

   if (!checkViewport()) {
      gLog->warn("Skipping draw with invalid viewport.");
      return false;
   }

   return true;
}

static gl::GLenum
getPrimitiveMode(latte::VGT_DI_PRIMITIVE_TYPE type)
{
   switch (type) {
   case latte::VGT_DI_PT_POINTLIST:
      return gl::GL_POINTS;
   case latte::VGT_DI_PT_LINELIST:
      return gl::GL_LINES;
   case latte::VGT_DI_PT_LINESTRIP:
      return gl::GL_LINE_STRIP;
   case latte::VGT_DI_PT_TRILIST:
   case latte::VGT_DI_PT_QUADLIST:  // Quads are rendered as triangles
   case latte::VGT_DI_PT_RECTLIST:  // Rects are rendered as triangles
      return gl::GL_TRIANGLES;
   case latte::VGT_DI_PT_TRIFAN:
      return gl::GL_TRIANGLE_FAN;
   case latte::VGT_DI_PT_TRISTRIP:
      return gl::GL_TRIANGLE_STRIP;
   case latte::VGT_DI_PT_LINELOOP:
      return gl::GL_LINE_LOOP;
   default:
      decaf_abort(fmt::format("Unimplemented VGT_PRIMITIVE_TYPE {}", type));
   }
}

template<typename IndexType>
static void
drawPrimitives2(gl::GLenum mode,
                uint32_t count,
                const IndexType *indices,
                uint32_t baseVertex,
                uint32_t numInstances,
                uint32_t baseInstance)
{
   if (numInstances == 1) {
      if (!indices) {
         gl::glDrawArrays(mode, baseVertex, count);
      } else if (std::is_same<IndexType, uint16_t>()) {
         gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_SHORT, indices, baseVertex);
      } else if (std::is_same<IndexType, uint32_t>()) {
         gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_INT, indices, baseVertex);
      }
   } else {
      if (!indices) {
         gl::glDrawArraysInstancedBaseInstance(mode, 0, count, numInstances, baseInstance);
      } else if (std::is_same<IndexType, uint16_t>()) {
         gl::glDrawElementsInstancedBaseInstance(mode, count, gl::GL_UNSIGNED_SHORT, indices, numInstances, baseInstance);
      } else if (std::is_same<IndexType, uint32_t>()) {
         gl::glDrawElementsInstancedBaseInstance(mode, count, gl::GL_UNSIGNED_INT, indices, numInstances, baseInstance);
      }
   }
}

template<bool IsRects, typename IndexType>
static void
unpackQuadRectList(uint32_t count,
               const IndexType *src,
               uint32_t baseVertex,
               uint32_t numInstances,
               uint32_t baseInstance)
{
   auto tris = (count / 4) * 6;
   auto unpacked = std::vector<IndexType>(tris);
   auto dst = &unpacked[0];

   // Unpack quad indices into triangle indices
   if (src) {
      for (auto i = 0u; i < count / 4; ++i) {
         auto index_0 = *src++;
         auto index_1 = *src++;
         auto index_2 = *src++;
         auto index_3 = *src++;

         *(dst++) = index_0;
         *(dst++) = index_1;
         *(dst++) = index_2;

         if (!IsRects) {
            *(dst++) = index_0;
            *(dst++) = index_2;
            *(dst++) = index_3;
         } else {
            // Rectangles use a different winding order apparently...
            *(dst++) = index_2;
            *(dst++) = index_1;
            *(dst++) = index_3;
         }
      }
   } else {
      auto index_0 = 0u;
      auto index_1 = 1u;
      auto index_2 = 2u;
      auto index_3 = 3u;

      for (auto i = 0u; i < count / 4; ++i) {
         auto index = i * 4;
         *(dst++) = index_0 + index;
         *(dst++) = index_1 + index;
         *(dst++) = index_2 + index;

         if (!IsRects) {
            *(dst++) = index_0 + index;
            *(dst++) = index_2 + index;
            *(dst++) = index_3 + index;
         } else {
            // Rectangles use a different winding order apparently...
            *(dst++) = index_2 + index;
            *(dst++) = index_1 + index;
            *(dst++) = index_3 + index;
         }
      }
   }

   drawPrimitives2(gl::GL_TRIANGLES, tris, unpacked.data(), baseVertex, numInstances, baseInstance);
}

void
GLDriver::drawPrimitives(uint32_t count,
                         const void *indices,
                         latte::VGT_INDEX indexFmt)
{
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto vgt_dma_num_instances = getRegister<latte::VGT_DMA_NUM_INSTANCES>(latte::Register::VGT_DMA_NUM_INSTANCES);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);

   auto primType = vgt_primitive_type.PRIM_TYPE();
   auto baseVertex = sq_vtx_base_vtx_loc.OFFSET();
   auto numInstances = vgt_dma_num_instances.NUM_INSTANCES();
   auto baseInstance = 0;

   auto mode = getPrimitiveMode(primType);

   if (vgt_strmout_en.STREAMOUT()) {
      auto baseMode = mode;

      if (mode == gl::GL_TRIANGLE_STRIP || mode == gl::GL_TRIANGLE_FAN) {
         baseMode = gl::GL_TRIANGLES;
      } else if (mode == gl::GL_LINE_STRIP || mode == gl::GL_LINE_LOOP) {
         baseMode = gl::GL_LINES;
      }

      if (!mFeedbackActive || mFeedbackPrimitive != baseMode) {
         if (mFeedbackActive) {
            gLog->warn("Primitive type changed during transform feedback");
            endTransformFeedback();
         }
         beginTransformFeedback(baseMode);
      } else {
         gl::glResumeTransformFeedback();
      }
   }

   if (primType == latte::VGT_DI_PT_QUADLIST) {
      if (indexFmt == latte::VGT_INDEX_16) {
         unpackQuadRectList<false>(count, reinterpret_cast<const uint16_t*>(indices), baseVertex, numInstances, baseInstance);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         unpackQuadRectList<false>(count, reinterpret_cast<const uint32_t*>(indices), baseVertex, numInstances, baseInstance);
      }
   } else if (primType == latte::VGT_DI_PT_RECTLIST) {
      if (indexFmt == latte::VGT_INDEX_16) {
         unpackQuadRectList<true>(count, reinterpret_cast<const uint16_t*>(indices), baseVertex, numInstances, baseInstance);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         unpackQuadRectList<true>(count, reinterpret_cast<const uint32_t*>(indices), baseVertex, numInstances, baseInstance);
      }
   } else {
      if (indexFmt == latte::VGT_INDEX_16) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint16_t*>(indices), baseVertex, numInstances, baseInstance);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint32_t*>(indices), baseVertex, numInstances, baseInstance);
      }
   }

   if (vgt_strmout_en.STREAMOUT()) {
      gl::glPauseTransformFeedback();
   }
}

void
GLDriver::drawPrimitivesIndexed(const void *buffer,
                                uint32_t count)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);
   auto vgt_dma_num_instances = getRegister<latte::VGT_DMA_NUM_INSTANCES>(latte::Register::VGT_DMA_NUM_INSTANCES);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);

   // Swap and indexBytes are separate because you can have 32-bit swap,
   //   but 16-bit indices in some cases...  This is also why we pre-swap
   //   the data before intercepting QUAD and POLYGON draws.
   if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_16_BIT) {
      auto *src = reinterpret_cast<const uint16_t *>(buffer);
      auto indices = std::vector<uint16_t>(count);

      if (vgt_dma_index_type.INDEX_TYPE() != latte::VGT_INDEX_16) {
         decaf_abort(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_16_BIT", vgt_dma_index_type.INDEX_TYPE()));
      }

      for (auto i = 0u; i < count; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(count,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE());
   } else if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_32_BIT) {
      auto *src = reinterpret_cast<const uint32_t *>(buffer);
      auto indices = std::vector<uint32_t>(count);

      if (vgt_dma_index_type.INDEX_TYPE() != latte::VGT_INDEX_32) {
         decaf_abort(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_32_BIT", vgt_dma_index_type.INDEX_TYPE()));
      }

      for (auto i = 0u; i < count; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(count,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE());
   } else if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_NONE) {
      drawPrimitives(count,
                     buffer,
                     vgt_dma_index_type.INDEX_TYPE());
   } else {
      decaf_abort(fmt::format("Unimplemented vgt_dma_index_type.SWAP_MODE {}", vgt_dma_index_type.SWAP_MODE()));
   }
}

void
GLDriver::drawIndexAuto(const pm4::DrawIndexAuto &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   drawPrimitives(data.count,
                  nullptr,
                  latte::VGT_INDEX_32);
}

void
GLDriver::drawIndex2(const pm4::DrawIndex2 &data)
{
   drawPrimitivesIndexed(data.addr, data.count);
}

void
GLDriver::drawIndexImmd(const pm4::DrawIndexImmd &data)
{
   drawPrimitivesIndexed(data.indices.data(), data.count);
}

void
GLDriver::decafClearColor(const pm4::DecafClearColor &data)
{
   float colors[] = {
      data.red,
      data.green,
      data.blue,
      data.alpha
   };

   // Find our colorbuffer to clear
   auto buffer = getColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info, true);

   // Bind color buffer
   gl::glNamedFramebufferTexture(mColorClearFrameBuffer, gl::GL_COLOR_ATTACHMENT0, buffer->active->object, 0);

   // Check color frame buffer is complete
   auto status = gl::glCheckNamedFramebufferStatus(mColorClearFrameBuffer, gl::GL_DRAW_FRAMEBUFFER);

   if (status != gl::GL_FRAMEBUFFER_COMPLETE) {
      gLog->warn("Skipping clear with invalid color buffer, status {}.", glbinding::Meta::getString(status));
      return;
   }

   // Clear color buffer
   glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   mColorBufferCache[0].mask = 0xF; // Recheck mask on next color buffer update
   gl::glDisable(gl::GL_SCISSOR_TEST);

   gl::glClearNamedFramebufferfv(mColorClearFrameBuffer, gl::GL_COLOR, 0, colors);

   gl::glEnable(gl::GL_SCISSOR_TEST);
}

void
GLDriver::decafClearDepthStencil(const pm4::DecafClearDepthStencil &data)
{
   auto db_depth_clear = getRegister<latte::DB_DEPTH_CLEAR>(latte::Register::DB_DEPTH_CLEAR);
   auto db_stencil_clear = getRegister<latte::DB_STENCIL_CLEAR>(latte::Register::DB_STENCIL_CLEAR);
   auto dbFormat = data.db_depth_info.FORMAT();
   auto hasStencil = (dbFormat == latte::DEPTH_8_24
                   || dbFormat == latte::DEPTH_8_24_FLOAT
                   || dbFormat == latte::DEPTH_X24_8_32_FLOAT);

   // Find our depthbuffer to clear
   auto buffer = getDepthBuffer(data.db_depth_base, data.db_depth_size, data.db_depth_info, true);

   // Bind depth buffer
   if (hasStencil) {
      gl::glNamedFramebufferTexture(mDepthClearFrameBuffer, gl::GL_DEPTH_STENCIL_ATTACHMENT, buffer->active->object, 0);
   } else {
      gl::glNamedFramebufferTexture(mDepthClearFrameBuffer, gl::GL_STENCIL_ATTACHMENT, 0, 0);
      gl::glNamedFramebufferTexture(mDepthClearFrameBuffer, gl::GL_DEPTH_ATTACHMENT, buffer->active->object, 0);
   }

   // Check depth frame buffer is complete
   auto status = gl::glCheckNamedFramebufferStatus(mDepthClearFrameBuffer, gl::GL_DRAW_FRAMEBUFFER);

   if (status != gl::GL_FRAMEBUFFER_COMPLETE) {
      gLog->warn("Skipping clear with invalid depth buffer, status {}.", glbinding::Meta::getString(status));
      return;
   }

   // Clear depth buffer
   auto db_depth_control = getRegister<latte::DB_DEPTH_CONTROL>(latte::Register::DB_DEPTH_CONTROL);

   if (!db_depth_control.Z_WRITE_ENABLE()) {
      gl::glDepthMask(gl::GL_TRUE);
   }

   gl::glDisable(gl::GL_SCISSOR_TEST);

   if (hasStencil) {
      gl::glClearNamedFramebufferfi(mDepthClearFrameBuffer, gl::GL_DEPTH_STENCIL, 0, db_depth_clear.DEPTH_CLEAR, db_stencil_clear.CLEAR());
   } else {
      gl::glClearNamedFramebufferfv(mDepthClearFrameBuffer, gl::GL_DEPTH, 0, &db_depth_clear.DEPTH_CLEAR);
   }

   gl::glEnable(gl::GL_SCISSOR_TEST);

   if (!db_depth_control.Z_WRITE_ENABLE()) {
      gl::glDepthMask(gl::GL_FALSE);
   }
}

} // namespace opengl

} // namespace gpu
