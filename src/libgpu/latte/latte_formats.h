#pragma once
#include <array>
#include <libgpu/latte/latte_enum_cb.h>
#include <libgpu/latte/latte_enum_db.h>
#include <libgpu/latte/latte_enum_sq.h>
#include <libgpu/latte/latte_enum_common.h>
#include <string>

namespace latte
{

struct SurfaceFormat
{
   latte::SQ_DATA_FORMAT format;
   latte::SQ_NUM_FORMAT numFormat;
   latte::SQ_FORMAT_COMP formatComp;
   uint32_t degamma;
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
getColorBufferDataFormat(latte::CB_FORMAT format, latte::CB_NUMBER_TYPE numberType);

SurfaceFormat
getDepthBufferDataFormat(latte::DB_FORMAT format);

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
