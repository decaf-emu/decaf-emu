#include "opengl_driver.h"
#include <glbinding/gl/gl.h>

namespace gpu
{

namespace opengl
{

bool GLDriver::checkReadyDraw()
{
   if (!checkActiveColorBuffer()) {
      gLog->warn("Skipping draw with invalid color buffer.");
      return false;
   }

   if (!checkActiveDepthBuffer()) {
      gLog->warn("Skipping draw with invalid depth buffer.");
      return false;
   }

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

   if (!checkActiveTextures()) {
      gLog->warn("Skipping draw with invalid textures.");
      return false;
   }

   if (!checkActiveSamplers()) {
      gLog->warn("Skipping draw with invalid samplers.");
      return false;
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
      return gl::GL_TRIANGLES;
   case latte::VGT_DI_PT_TRIFAN:
      return gl::GL_TRIANGLE_FAN;
   case latte::VGT_DI_PT_TRISTRIP:
      return gl::GL_TRIANGLE_STRIP;
   case latte::VGT_DI_PT_LINELOOP:
      return gl::GL_LINE_LOOP;
   default:
      throw unimplemented_error(fmt::format("Unimplemented VGT_PRIMITIVE_TYPE {}", type));
   }
}

template<typename IndexType>
static void
drawPrimitives2(gl::GLenum mode,
                uint32_t count,
                const IndexType *indices,
                uint32_t offset)
{
   if (!indices) {
      gl::glDrawArrays(mode, offset, count);
   } else if (std::is_same<IndexType, uint16_t>()) {
      gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_SHORT, indices, offset);
   } else if (std::is_same<IndexType, uint32_t>()) {
      gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_INT, indices, offset);
   }
}

template<typename IndexType>
static void
unpackQuadList(uint32_t count,
               const IndexType *src,
               uint32_t offset)
{
   auto tris = (count / 4) * 6;
   auto unpacked = std::vector<IndexType>(tris);
   auto dst = &unpacked[0];

   // Unpack quad indices into triangle indices
   if (src) {
      for (auto i = 0u; i < count / 4; ++i) {
         auto index_tl = *src++;
         auto index_tr = *src++;
         auto index_br = *src++;
         auto index_bl = *src++;

         *(dst++) = index_tl;
         *(dst++) = index_tr;
         *(dst++) = index_bl;

         *(dst++) = index_bl;
         *(dst++) = index_tr;
         *(dst++) = index_br;
      }
   } else {
      auto index_tl = 0u;
      auto index_tr = 1u;
      auto index_br = 2u;
      auto index_bl = 3u;

      for (auto i = 0u; i < count / 4; ++i) {
         auto index = i * 4;
         *(dst++) = index_tl + index;
         *(dst++) = index_tr + index;
         *(dst++) = index_bl + index;

         *(dst++) = index_bl + index;
         *(dst++) = index_tr + index;
         *(dst++) = index_br + index;
      }
   }

   drawPrimitives2(gl::GL_TRIANGLES, tris, unpacked.data(), offset);
}

static void
drawPrimitives(latte::VGT_DI_PRIMITIVE_TYPE primType,
               uint32_t offset,
               uint32_t count,
               const void *indices,
               latte::VGT_INDEX indexFmt)
{
   if (primType == latte::VGT_DI_PT_QUADLIST) {
      if (indexFmt == latte::VGT_INDEX_16) {
         unpackQuadList(count, reinterpret_cast<const uint16_t*>(indices), offset);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         unpackQuadList(count, reinterpret_cast<const uint32_t*>(indices), offset);
      }
   } else {
      auto mode = getPrimitiveMode(primType);

      if (indexFmt == latte::VGT_INDEX_16) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint16_t*>(indices), offset);
      } else if (indexFmt == latte::VGT_INDEX_32) {
         drawPrimitives2(mode, count, reinterpret_cast<const uint32_t*>(indices), offset);
      }
   }
}

void
GLDriver::drawIndexAuto(const pm4::DrawIndexAuto &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);

   drawPrimitives(vgt_primitive_type.PRIM_TYPE(),
                  sq_vtx_base_vtx_loc.OFFSET,
                  data.count,
                  nullptr,
                  latte::VGT_INDEX_32);
}

void
GLDriver::drawIndex2(const pm4::DrawIndex2 &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);

   // Swap and indexBytes are separate because you can have 32-bit swap,
   //   but 16-bit indices in some cases...  This is also why we pre-swap
   //   the data before intercepting QUAD and POLYGON draws.
   if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_16_BIT) {
      auto *src = static_cast<uint16_t*>(data.addr.get());
      auto indices = std::vector<uint16_t>(data.count);

      if (vgt_dma_index_type.INDEX_TYPE() != latte::VGT_INDEX_16) {
         throw std::logic_error(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_16_BIT", vgt_dma_index_type.INDEX_TYPE()));
      }

      for (auto i = 0u; i < data.count; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(vgt_primitive_type.PRIM_TYPE(),
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.count,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE());
   } else if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_32_BIT) {
      auto *src = static_cast<uint32_t*>(data.addr.get());
      auto indices = std::vector<uint32_t>(data.count);

      if (vgt_dma_index_type.INDEX_TYPE() != latte::VGT_INDEX_32) {
         throw std::logic_error(fmt::format("Unexpected INDEX_TYPE {} for VGT_DMA_SWAP_32_BIT", vgt_dma_index_type.INDEX_TYPE()));
      }

      for (auto i = 0u; i < data.count; ++i) {
         indices[i] = byte_swap(src[i]);
      }

      drawPrimitives(vgt_primitive_type.PRIM_TYPE(),
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.count,
                     indices.data(),
                     vgt_dma_index_type.INDEX_TYPE());
   } else if (vgt_dma_index_type.SWAP_MODE() == latte::VGT_DMA_SWAP_NONE) {
      drawPrimitives(vgt_primitive_type.PRIM_TYPE(),
                     sq_vtx_base_vtx_loc.OFFSET,
                     data.count,
                     data.addr,
                     vgt_dma_index_type.INDEX_TYPE());
   } else {
      throw unimplemented_error(fmt::format("Unimplemented vgt_dma_index_type.SWAP_MODE {}", vgt_dma_index_type.SWAP_MODE()));
   }
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
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Bind color buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, buffer->object, 0);

   // Clear color buffer
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glClearBufferfv(gl::GL_COLOR, 0, colors);
   gl::glEnable(gl::GL_SCISSOR_TEST);

   // Restore original color buffer
   if (mActiveColorBuffers[0]) {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, mActiveColorBuffers[0]->object, 0);
   } else {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, 0, 0);
   }
}

void
GLDriver::decafClearDepthStencil(const pm4::DecafClearDepthStencil &data)
{
   auto db_depth_clear = getRegister<latte::DB_DEPTH_CLEAR>(latte::Register::DB_DEPTH_CLEAR);
   auto db_stencil_clear = getRegister<latte::DB_STENCIL_CLEAR>(latte::Register::DB_STENCIL_CLEAR);

   // Find our depthbuffer to clear
   auto db_depth_base = bit_cast<latte::DB_DEPTH_BASE>(data.bufferAddr);
   auto buffer = getDepthBuffer(db_depth_base, data.db_depth_size, data.db_depth_info);

   // Bind depth buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, buffer->object, 0);

   // Clear depth buffer
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glClearBufferfi(gl::GL_DEPTH_STENCIL, 0, db_depth_clear.DEPTH_CLEAR, db_stencil_clear.CLEAR());
   gl::glEnable(gl::GL_SCISSOR_TEST);

   // Restore original depth buffer
   if (mActiveDepthBuffer) {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, mActiveDepthBuffer->object, 0);
   } else {
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, 0, 0);
   }
}

} // namespace opengl

} // namespace gpu
