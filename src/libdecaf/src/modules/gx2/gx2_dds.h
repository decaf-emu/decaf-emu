#pragma once
#include <string>

namespace gx2
{

struct GX2Surface;

namespace debug
{

bool
saveDDS(const std::string &filename,
        const GX2Surface *surface,
        const void *imagePtr,
        const void *mipPtr);

} // namespace debug

} // namespace gx2
