#pragma once
#include <cstdint>

#define RADEON_BO_FLAGS_MACRO_TILE  1
#define RADEON_BO_FLAGS_MICRO_TILE  2

namespace mesa
{

struct radeon_renderbuffer
{
   struct
   {
      int Width;
      int Height;
   } base;

   struct data
   {
      uint8_t *ptr;
      uint32_t flags;
   } *bo;

   bool has_surface;
   int pitch;
   int cpp;	// byte per pixel
   int group_bytes;
   int num_channels;	// same as pipes in r800 and above
   int num_banks;
   int r7xx_bank_op;
};

uint8_t *
r600_ptr_depth(const struct radeon_renderbuffer * rrb, int x, int y);

uint8_t *
r600_ptr_stencil(const struct radeon_renderbuffer * rrb, int x, int y);

uint8_t *
r600_ptr_color(const struct radeon_renderbuffer * rrb, int x, int y);

} // namespace mesa
