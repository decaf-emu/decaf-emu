#pragma once
#include "decaf_game.h"

namespace kernel
{

bool
loadAppXML(const char *path,
           decaf::AppXML &app);

bool
loadCosXML(const char *path,
           decaf::CosXML &cos);

bool
loadMetaXML(const char *path,
            decaf::MetaXML &meta);

bool
loadGameInfo(decaf::GameInfo &info);

} // namespace kernel
