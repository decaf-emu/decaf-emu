#pragma once
#include <array>
#include <libgpu/latte/latte_enum_sq.h>
#include <libgpu/latte/latte_enum_common.h>
#include <string>

namespace latte
{

uint32_t
getDataFormatBitsPerElement(latte::SQ_DATA_FORMAT format);

bool
getDataFormatIsCompressed(latte::SQ_DATA_FORMAT format);

std::string
getDataFormatName(latte::SQ_DATA_FORMAT format);

uint32_t
getDataFormatComponents(latte::SQ_DATA_FORMAT format);

uint32_t
getDataFormatComponentBits(latte::SQ_DATA_FORMAT format);

bool
getDataFormatIsFloat(latte::SQ_DATA_FORMAT format);

latte::SQ_TILE_MODE
getArrayModeTileMode(latte::BUFFER_ARRAY_MODE mode);

} // namespace gpu
