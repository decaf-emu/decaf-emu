#pragma once
#include <string>

struct GX2Surface;

namespace gx2
{

namespace debug
{

bool
saveDDS(const std::string &filename,
        const GX2Surface *surface,
        const void *imagePtr,
        const void *mipPtr);

} // namespace debug

} // namespace gx2
