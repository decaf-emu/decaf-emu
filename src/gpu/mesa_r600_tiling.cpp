#include "mesa_r600_tiling.h"

typedef int GLint;
typedef unsigned char GLubyte;

// Following code is derived from the legacy Mesa r600 driver (radeon_span.c).
namespace mesa
{

/**************************************************************************
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
VA Linux Systems Inc., Fremont, California.
The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.
All Rights Reserved.
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:
The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************/

/*
* Authors:
*   Kevin E. Martin <martin@valinux.com>
*   Gareth Hughes <gareth@valinux.com>
*   Keith Whitwell <keith@tungstengraphics.com>
*
*/

static inline GLint
r600_coord_within_microtile(GLint x, GLint y, GLint element_bytes)
{
   GLint pixel_number = 0;
   switch (element_bytes) {
   case 1:
      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((x >> 1) & 1) << 1; // pn[1] = x[1]
      pixel_number |= ((x >> 2) & 1) << 2; // pn[2] = x[2]
      pixel_number |= ((y >> 1) & 1) << 3; // pn[3] = y[1]
      pixel_number |= ((y >> 0) & 1) << 4; // pn[4] = y[0]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      break;
   case 2:
      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((x >> 1) & 1) << 1; // pn[1] = x[1]
      pixel_number |= ((x >> 2) & 1) << 2; // pn[2] = x[2]
      pixel_number |= ((y >> 0) & 1) << 3; // pn[3] = y[0]
      pixel_number |= ((y >> 1) & 1) << 4; // pn[4] = y[1]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      break;
   case 4:
      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((x >> 1) & 1) << 1; // pn[1] = x[1]
      pixel_number |= ((y >> 0) & 1) << 2; // pn[2] = y[0]
      pixel_number |= ((x >> 2) & 1) << 3; // pn[3] = x[2]
      pixel_number |= ((y >> 1) & 1) << 4; // pn[4] = y[1]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      break;
   case 8:
      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((y >> 0) & 1) << 1; // pn[1] = y[0]
      pixel_number |= ((x >> 1) & 1) << 2; // pn[2] = x[1]
      pixel_number |= ((x >> 2) & 1) << 3; // pn[3] = x[2]
      pixel_number |= ((y >> 1) & 1) << 4; // pn[4] = y[1]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      break;
   case 16:
      pixel_number |= ((y >> 0) & 1) << 0; // pn[0] = y[0]
      pixel_number |= ((x >> 0) & 1) << 1; // pn[1] = x[0]
      pixel_number |= ((x >> 1) & 1) << 2; // pn[2] = x[1]
      pixel_number |= ((x >> 2) & 1) << 3; // pn[3] = x[2]
      pixel_number |= ((y >> 1) & 1) << 4; // pn[4] = y[1]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      break;
   }
   return pixel_number;
}


static inline GLint
r600_1d_tile_helper(const struct radeon_renderbuffer * rrb,
                    GLint x, GLint y,
                    GLint is_depth,
                    GLint is_stencil)
{
   GLint element_bytes = rrb->cpp;
   GLint num_samples = 1;
   GLint tile_width = 8;
   GLint tile_height = 8;
   GLint tile_thickness = 1;
   GLint pitch_elements = rrb->pitch / element_bytes;
   GLint height = rrb->base.Height;
   GLint z = 0;
   GLint sample_number = 0;
   /* */
   GLint tile_bytes;
   GLint tiles_per_row;
   GLint tiles_per_slice;
   GLint slice_offset;
   GLint tile_row_index;
   GLint tile_column_index;
   GLint tile_offset;
   GLint pixel_number = 0;
   GLint element_offset;
   GLint offset = 0;

   tile_bytes = tile_width * tile_height * tile_thickness * element_bytes * num_samples;
   tiles_per_row = pitch_elements / tile_width;
   tiles_per_slice = tiles_per_row * (height / tile_height);
   slice_offset = (z / tile_thickness) * tiles_per_slice * tile_bytes;
   tile_row_index = y / tile_height;
   tile_column_index = x / tile_width;
   tile_offset = ((tile_row_index * tiles_per_row) + tile_column_index) * tile_bytes;

   if (is_depth) {
      GLint pixel_offset = 0;

      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((y >> 0) & 1) << 1; // pn[1] = y[0]
      pixel_number |= ((x >> 1) & 1) << 2; // pn[2] = x[1]
      pixel_number |= ((y >> 1) & 1) << 3; // pn[3] = y[1]
      pixel_number |= ((x >> 2) & 1) << 4; // pn[4] = x[2]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      switch (element_bytes) {
      case 2:
         pixel_offset = pixel_number * element_bytes * num_samples;
         break;
      case 4:
         /* stencil and depth data are stored separately within a tile.
         * stencil is stored in a contiguous tile before the depth tile.
         * stencil element is 1 byte, depth element is 3 bytes.
         * stencil tile is 64 bytes.
         */
         if (is_stencil)
            pixel_offset = pixel_number * 1 * num_samples;
         else
            pixel_offset = (pixel_number * 3 * num_samples) + 64;
         break;
      }
      element_offset = pixel_offset + (sample_number * element_bytes);
   } else {
      GLint sample_offset;

      pixel_number = r600_coord_within_microtile(x, y, element_bytes);
      sample_offset = sample_number * (tile_bytes / num_samples);
      element_offset = sample_offset + (pixel_number * element_bytes);
   }
   offset = slice_offset + tile_offset + element_offset;
   return offset;
}


static inline GLint
r600_log2(GLint n)
{
   GLint log2 = 0;

   while (n >>= 1)
      ++log2;
   return log2;
}


static inline GLint
r600_2d_tile_helper(const struct radeon_renderbuffer * rrb,
                    GLint x, GLint y,
                    GLint is_depth,
                    GLint is_stencil)
{
   GLint group_bytes = rrb->group_bytes;
   GLint num_channels = rrb->num_channels;
   GLint num_banks = rrb->num_banks;
   GLint r7xx_bank_op = rrb->r7xx_bank_op;
   /* */
   GLint group_bits = r600_log2(group_bytes);
   GLint channel_bits = r600_log2(num_channels);
   GLint bank_bits = r600_log2(num_banks);
   GLint element_bytes = rrb->cpp;
   GLint num_samples = 1;
   GLint tile_width = 8;
   GLint tile_height = 8;
   GLint tile_thickness = 1;
   GLint macro_tile_width = num_banks;
   GLint macro_tile_height = num_channels;
   GLint pitch_elements = (rrb->pitch / element_bytes) / tile_width;
   GLint height = rrb->base.Height / tile_height;
   GLint z = 0;
   GLint sample_number = 0;
   /* */
   GLint tile_bytes;
   GLint macro_tile_bytes;
   GLint macro_tiles_per_row;
   GLint macro_tiles_per_slice;
   GLint slice_offset;
   GLint macro_tile_row_index;
   GLint macro_tile_column_index;
   GLint macro_tile_offset;
   GLint pixel_number = 0;
   GLint element_offset;
   GLint bank = 0;
   GLint channel = 0;
   GLint total_offset;
   GLint group_mask = (1 << group_bits) - 1;
   GLint offset_low;
   GLint offset_high;
   GLint offset = 0;

   switch (num_channels) {
   case 2:
   default:
      // channel[0] = x[3] ^ y[3]
      channel |= (((x >> 3) ^ (y >> 3)) & 1) << 0;
      break;
   case 4:
      // channel[0] = x[4] ^ y[3]
      channel |= (((x >> 4) ^ (y >> 3)) & 1) << 0;
      // channel[1] = x[3] ^ y[4]
      channel |= (((x >> 3) ^ (y >> 4)) & 1) << 1;
      break;
   case 8:
      // channel[0] = x[5] ^ y[3]
      channel |= (((x >> 5) ^ (y >> 3)) & 1) << 0;
      // channel[0] = x[4] ^ x[5] ^ y[4]
      channel |= (((x >> 4) ^ (x >> 5) ^ (y >> 4)) & 1) << 1;
      // channel[0] = x[3] ^ y[5]
      channel |= (((x >> 3) ^ (y >> 5)) & 1) << 2;
      break;
   }

   switch (num_banks) {
   case 4:
      // bank[0] = x[3] ^ y[4 + log2(num_channels)]
      bank |= (((x >> 3) ^ (y >> (4 + channel_bits))) & 1) << 0;
      if (r7xx_bank_op)
         // bank[1] = x[3] ^ y[4 + log2(num_channels)] ^ x[5]
         bank |= (((x >> 4) ^ (y >> (3 + channel_bits)) ^ (x >> 5)) & 1) << 1;
      else
         // bank[1] = x[4] ^ y[3 + log2(num_channels)]
         bank |= (((x >> 4) ^ (y >> (3 + channel_bits))) & 1) << 1;
      break;
   case 8:
      // bank[0] = x[3] ^ y[5 + log2(num_channels)]
      bank |= (((x >> 3) ^ (y >> (5 + channel_bits))) & 1) << 0;
      // bank[1] = x[4] ^ y[4 + log2(num_channels)] ^ y[5 + log2(num_channels)]
      bank |= (((x >> 4) ^ (y >> (4 + channel_bits)) ^ (y >> (5 + channel_bits))) & 1) << 1;
      if (r7xx_bank_op)
         // bank[2] = x[5] ^ y[3 + log2(num_channels)] ^ x[6]
         bank |= (((x >> 5) ^ (y >> (3 + channel_bits)) ^ (x >> 6)) & 1) << 2;
      else
         // bank[2] = x[5] ^ y[3 + log2(num_channels)]
         bank |= (((x >> 5) ^ (y >> (3 + channel_bits))) & 1) << 2;
      break;
   }

   tile_bytes = tile_width * tile_height * tile_thickness * element_bytes * num_samples;
   macro_tile_bytes = macro_tile_width * macro_tile_height * tile_bytes;
   macro_tiles_per_row = pitch_elements / macro_tile_width;
   macro_tiles_per_slice = macro_tiles_per_row * (height / macro_tile_height);
   slice_offset = (z / tile_thickness) * macro_tiles_per_slice * macro_tile_bytes;
   macro_tile_row_index = (y / tile_height) / macro_tile_height;
   macro_tile_column_index = (x / tile_width) / macro_tile_width;
   macro_tile_offset = ((macro_tile_row_index * macro_tiles_per_row) + macro_tile_column_index) * macro_tile_bytes;

   if (is_depth) {
      GLint pixel_offset = 0;

      pixel_number |= ((x >> 0) & 1) << 0; // pn[0] = x[0]
      pixel_number |= ((y >> 0) & 1) << 1; // pn[1] = y[0]
      pixel_number |= ((x >> 1) & 1) << 2; // pn[2] = x[1]
      pixel_number |= ((y >> 1) & 1) << 3; // pn[3] = y[1]
      pixel_number |= ((x >> 2) & 1) << 4; // pn[4] = x[2]
      pixel_number |= ((y >> 2) & 1) << 5; // pn[5] = y[2]
      switch (element_bytes) {
      case 2:
         pixel_offset = pixel_number * element_bytes * num_samples;
         break;
      case 4:
         /* stencil and depth data are stored separately within a tile.
         * stencil is stored in a contiguous tile before the depth tile.
         * stencil element is 1 byte, depth element is 3 bytes.
         * stencil tile is 64 bytes.
         */
         if (is_stencil)
            pixel_offset = pixel_number * 1 * num_samples;
         else
            pixel_offset = (pixel_number * 3 * num_samples) + 64;
         break;
      }
      element_offset = pixel_offset + (sample_number * element_bytes);
   } else {
      GLint sample_offset;

      pixel_number = r600_coord_within_microtile(x, y, element_bytes);

      sample_offset = sample_number * (tile_bytes / num_samples);
      element_offset = sample_offset + (pixel_number * element_bytes);
   }
   total_offset = (slice_offset + macro_tile_offset) >> (channel_bits + bank_bits);
   total_offset += element_offset;

   offset_low = total_offset & group_mask;
   offset_high = (total_offset & ~group_mask) << (channel_bits + bank_bits);
   offset = (bank << (group_bits + channel_bits)) + (channel << group_bits) + offset_low + offset_high;

   return offset;
}


uint8_t *
r600_ptr_depth(const struct radeon_renderbuffer * rrb, int x, int y)
{
   GLubyte *ptr = rrb->bo->ptr;
   GLint offset;
   if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
      offset = r600_2d_tile_helper(rrb, x, y, 1, 0);
   else
      offset = r600_1d_tile_helper(rrb, x, y, 1, 0);
   return &ptr[offset];
}


uint8_t *
r600_ptr_stencil(const struct radeon_renderbuffer * rrb, int x, int y)
{
   GLubyte *ptr = rrb->bo->ptr;
   GLint offset;
   if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
      offset = r600_2d_tile_helper(rrb, x, y, 1, 1);
   else
      offset = r600_1d_tile_helper(rrb, x, y, 1, 1);
   return &ptr[offset];
}


uint8_t *
r600_ptr_color(const struct radeon_renderbuffer * rrb, int x, int y)
{
   GLubyte *ptr = rrb->bo->ptr;
   uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
   GLint offset;
   if (rrb->has_surface || !(rrb->bo->flags & mask)) {
      offset = x * rrb->cpp + y * rrb->pitch;
   } else {
      if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE)
         offset = r600_2d_tile_helper(rrb, x, y, 0, 0);
      else
         offset = r600_1d_tile_helper(rrb, x, y, 0, 0);
   }
   return &ptr[offset];
}

} // namespace mesa
