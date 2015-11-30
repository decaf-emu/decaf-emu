#include "latte_format.h"
#include "modules/gx2/gx2_format.h"

namespace latte
{

size_t
formatBytesPerElement(SQ_DATA_FORMAT format)
{
   return GX2GetSurfaceElementBytes(static_cast<GX2SurfaceFormat::Value>(format));
}

} // namespace latte
