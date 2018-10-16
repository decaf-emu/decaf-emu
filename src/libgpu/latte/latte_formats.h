#pragma once
#include <array>
#include <libgpu/latte/latte_enum_cb.h>
#include <libgpu/latte/latte_enum_db.h>
#include <libgpu/latte/latte_enum_sq.h>
#include <libgpu/latte/latte_enum_common.h>
#include <string>

namespace latte
{

enum SurfaceFormat : uint32_t
{
   Invalid           = 0x0000, // FMT_INVALID
   R8Unorm           = 0x0001, // FMT_8
   R8Uint            = 0x0101, // FMT_8
   R8Snorm           = 0x0201, // FMT_8
   R8Sint            = 0x0301, // FMT_8
   R4G4Unorm         = 0x0002, // FMT_4_4
   R16Unorm          = 0x0005, // FMT_16
   R16Uint           = 0x0105, // FMT_16
   R16Snorm          = 0x0205, // FMT_16
   R16Sint           = 0x0305, // FMT_16
   R16Float          = 0x0806, // FMT_16_FLOAT
   R8G8Unorm         = 0x0007, // FMT_8_8
   R8G8Uint          = 0x0107, // FMT_8_8
   R8G8Snorm         = 0x0207, // FMT_8_8
   R8G8Sint          = 0x0307, // FMT_8_8
   R5G6B5Unorm       = 0x0008, // FMT_5_6_5
   R5G5B5A1Unorm     = 0x000a, // FMT_1_5_5_5
   R4G4B4A4Unorm     = 0x000b, // FMT_4_4_4_4
   A1B5G5R5Unorm     = 0x000c, // FMT_5_5_5_1
   R32Uint           = 0x010d, // FMT_32
   R32Sint           = 0x030d, // FMT_32
   R32Float          = 0x080e, // FMT_32_FLOAT
   R16G16Unorm       = 0x000f, // FMT_16_16
   R16G16Uint        = 0x010f, // FMT_16_16
   R16G16Snorm       = 0x020f, // FMT_16_16
   R16G16Sint        = 0x030f, // FMT_16_16
   R16G16Float       = 0x0810, // FMT_16_16_FLOAT
   D24UnormS8Uint    = 0x0011, // FMT_8_24
   X24G8Uint         = 0x0111, // FMT_8_24
   R11G11B10Float    = 0x0816, // FMT_10_11_11_FLOAT
   R10G10B10A2Unorm  = 0x0019, // FMT_2_10_10_10
   R10G10B10A2Uint   = 0x0119, // FMT_2_10_10_10
   R10G10B10A2Snorm  = 0x0219, // FMT_2_10_10_10
   R10G10B10A2Sint   = 0x0319, // FMT_2_10_10_10
   R8G8B8A8Unorm     = 0x001a, // FMT_8_8_8_8
   R8G8B8A8Uint      = 0x011a, // FMT_8_8_8_8
   R8G8B8A8Snorm     = 0x021a, // FMT_8_8_8_8
   R8G8B8A8Sint      = 0x031a, // FMT_8_8_8_8
   R8G8B8A8Srgb      = 0x041a, // FMT_8_8_8_8
   A2B10G10R10Unorm  = 0x001b, // FMT_10_10_10_2
   A2B10G10R10Uint   = 0x011b, // FMT_10_10_10_2
   D32FloatS8UintX24 = 0x081c, // FMT_X24_8_32_FLOAT
   D32G8UintX24      = 0x011c, // FMT_X24_8_32_FLOAT
   R32G32Uint        = 0x011d, // FMT_32_32
   R32G32Sint        = 0x031d, // FMT_32_32
   R32G32Float       = 0x081e, // FMT_32_32_FLOAT
   R16G16B16A16Unorm = 0x001f, // FMT_16_16_16_16
   R16G16B16A16Uint  = 0x011f, // FMT_16_16_16_16
   R16G16B16A16Snorm = 0x021f, // FMT_16_16_16_16
   R16G16B16A16Sint  = 0x031f, // FMT_16_16_16_16
   R16G16B16A16Float = 0x0820, // FMT_16_16_16_16_FLOAT
   R32G32B32A32Uint  = 0x0122, // FMT_32_32_32_32
   R32G32B32A32Sint  = 0x0322, // FMT_32_32_32_32
   R32G32B32A32Float = 0x0823, // FMT_32_32_32_32_FLOAT
   BC1Unorm          = 0x0031, // FMT_BC1
   BC1Srgb           = 0x0431, // FMT_BC1
   BC2Unorm          = 0x0032, // FMT_BC2
   BC2Srgb           = 0x0432, // FMT_BC2
   BC3Unorm          = 0x0033, // FMT_BC3
   BC3Srgb           = 0x0433, // FMT_BC3
   BC4Unorm          = 0x0034, // FMT_BC4
   BC4Snorm          = 0x0234, // FMT_BC4
   BC5Unorm          = 0x0035, // FMT_BC5
   BC5Snorm          = 0x0235, // FMT_BC5
   NV12              = 0x0081, // FMT_NV12
};

enum class DataFormatMetaType : uint32_t
{
   None,
   UINT,
   FLOAT
};

struct DataFormatMetaElem
{
   uint32_t index;
   uint32_t start;
   uint32_t length;
};

struct DataFormatMeta
{
   uint32_t inputWidth;
   uint32_t inputCount;
   DataFormatMetaType type;
   DataFormatMetaElem elems[4];
};

SurfaceFormat
getSurfaceFormat(latte::SQ_DATA_FORMAT dataFormat,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 bool forceDegamma);

latte::SQ_DATA_FORMAT
getSurfaceFormatDataFormat(latte::SurfaceFormat format);

SurfaceFormat
getColorBufferSurfaceFormat(latte::CB_FORMAT format, latte::CB_NUMBER_TYPE numberType);

SurfaceFormat
getDepthBufferSurfaceFormat(latte::DB_FORMAT format);

uint32_t
getDataFormatBitsPerElement(latte::SQ_DATA_FORMAT format);

bool
getDataFormatIsCompressed(latte::SQ_DATA_FORMAT format);

std::string
getDataFormatName(latte::SQ_DATA_FORMAT format);

DataFormatMeta
getDataFormatMeta(latte::SQ_DATA_FORMAT format);

uint32_t
getDataFormatComponents(latte::SQ_DATA_FORMAT format);

uint32_t
getDataFormatComponentBits(latte::SQ_DATA_FORMAT format);

bool
getDataFormatIsFloat(latte::SQ_DATA_FORMAT format);

uint32_t
getTexDimDimensions(latte::SQ_TEX_DIM dim);

latte::SQ_TILE_MODE
getArrayModeTileMode(latte::BUFFER_ARRAY_MODE mode);

} // namespace gpu
