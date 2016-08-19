#pragma once
#include "common/types.h"

namespace latte
{

enum CB_BLEND_FUNC : uint32_t
{
   CB_BLEND_ZERO                       = 0,
   CB_BLEND_ONE                        = 1,
   CB_BLEND_SRC_COLOR                  = 2,
   CB_BLEND_ONE_MINUS_SRC_COLOR        = 3,
   CB_BLEND_SRC_ALPHA                  = 4,
   CB_BLEND_ONE_MINUS_SRC_ALPHA        = 5,
   CB_BLEND_DST_ALPHA                  = 6,
   CB_BLEND_ONE_MINUS_DST_ALPHA        = 7,
   CB_BLEND_DST_COLOR                  = 8,
   CB_BLEND_ONE_MINUS_DST_COLOR        = 9,
   CB_BLEND_SRC_ALPHA_SATURATE         = 10,
   CB_BLEND_BOTH_SRC_ALPHA             = 11,
   CB_BLEND_BOTH_INV_SRC_ALPHA         = 12,
   CB_BLEND_CONSTANT_COLOR             = 13,
   CB_BLEND_ONE_MINUS_CONSTANT_COLOR   = 14,
   CB_BLEND_SRC1_COLOR                 = 15,
   CB_BLEND_ONE_MINUS_SRC1_COLOR       = 16,
   CB_BLEND_SRC1_ALPHA                 = 17,
   CB_BLEND_ONE_MINUS_SRC1_ALPHA       = 18,
   CB_BLEND_CONSTANT_ALPHA             = 19,
   CB_BLEND_ONE_MINUS_CONSTANT_ALPHA   = 20,
};

enum CB_CLRCMP_DRAW : uint32_t
{
   CB_CLRCMP_DRAW_ALWAYS      = 0,
   CB_CLRCMP_DRAW_NEVER       = 1,
   CB_CLRCMP_DRAW_ON_NEQ      = 4,
   CB_CLRCMP_DRAW_ON_EQ       = 5,
};

enum CB_CLRCMP_SEL : uint32_t
{
   CB_CLRCMP_SEL_DST          = 0,
   CB_CLRCMP_SEL_SRC          = 1,
   CB_CLRCMP_SEL_AND          = 2,
};

enum CB_COMB_FUNC : uint32_t
{
   CB_COMB_DST_PLUS_SRC       = 0,
   CB_COMB_SRC_MINUS_DST      = 1,
   CB_COMB_MIN_DST_SRC        = 2,
   CB_COMB_MAX_DST_SRC        = 3,
   CB_COMB_DST_MINUS_SRC      = 4,
};

enum CB_ENDIAN : uint32_t
{
   CB_ENDIAN_NONE             = 0,
   CB_ENDIAN_8IN16            = 1,
   CB_ENDIAN_8IN32            = 2,
   CB_ENDIAN_8IN64            = 3,
};

enum CB_FORMAT : uint32_t
{
   COLOR_INVALID              = 0,
   COLOR_8                    = 1,
   COLOR_4_4                  = 2,
   COLOR_3_3_2                = 3,
   COLOR_16                   = 5,
   COLOR_16_FLOAT             = 6,
   COLOR_8_8                  = 7,
   COLOR_5_6_5                = 8,
   COLOR_6_5_5                = 9,
   COLOR_1_5_5_5              = 10,
   COLOR_4_4_4_4              = 11,
   COLOR_5_5_5_1              = 12,
   COLOR_32                   = 13,
   COLOR_32_FLOAT             = 14,
   COLOR_16_16                = 15,
   COLOR_16_16_FLOAT          = 16,
   COLOR_8_24                 = 17,
   COLOR_8_24_FLOAT           = 18,
   COLOR_24_8                 = 19,
   COLOR_24_8_FLOAT           = 20,
   COLOR_10_11_11             = 21,
   COLOR_10_11_11_FLOAT       = 22,
   COLOR_11_11_10             = 23,
   COLOR_11_11_10_FLOAT       = 24,
   COLOR_2_10_10_10           = 25,
   COLOR_8_8_8_8              = 26,
   COLOR_10_10_10_2           = 27,
   COLOR_X24_8_32_FLOAT       = 28,
   COLOR_32_32                = 29,
   COLOR_32_32_FLOAT          = 30,
   COLOR_16_16_16_16          = 31,
   COLOR_16_16_16_16_FLOAT    = 32,
   COLOR_32_32_32_32          = 34,
   COLOR_32_32_32_32_FLOAT    = 35,
};

enum CB_NUMBER_TYPE : uint32_t
{
   NUMBER_UNORM               = 0,
   NUMBER_SNORM               = 1,
   NUMBER_USCALED             = 2,
   NUMBER_SSCALED             = 3,
   NUMBER_UINT                = 4,
   NUMBER_SINT                = 5,
   NUMBER_SRGB                = 6,
   NUMBER_FLOAT               = 7,
};

enum CB_ROUND_MODE : uint32_t
{
   ROUND_BY_HALF              = 0,
   ROUND_TRUNCATE             = 1,
};

enum CB_SOURCE_FORMAT
{
   EXPORT_FULL                = 0,
   EXPORT_NORM                = 1,
};

enum CB_SPECIAL_OP : uint32_t
{
   CB_SPECIAL_NORMAL          = 0,
   CB_SPECIAL_DISABLE         = 1,
   CB_SPECIAL_FAST_CLEAR      = 2,
   CB_SPECIAL_FORCE_CLEAR     = 3,
   CB_SPECIAL_EXPAND_COLOR    = 4,
   CB_SPECIAL_EXPAND_TEXTURE  = 5,
   CB_SPECIAL_EXPAND_SAMPLES  = 6,
   CB_SPECIAL_RESOLVE_BOX     = 7,
};

enum CB_COMP_SWAP : uint32_t
{
   SWAP_STD                   = 0,
   SWAP_ALT                   = 1,
   SWAP_STD_REV               = 2,
   SWAP_ALT_REV               = 3,
};

enum CB_TILE_MODE : uint32_t
{
   TILE_DISABLE               = 0,
   TILE_CLEAR_ENABLE          = 1,
   TILE_FRAG_ENABLE           = 2,
};

} // namespace latte
