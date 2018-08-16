#pragma once
#include "cafe_hle_library.h"

#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::hle
{

void
initialiseLibraries();

Library *
getLibrary(LibraryId id);

Library *
getLibrary(std::string_view name);

void
relocateLibrary(std::string_view name,
                virt_addr textBaseAddress,
                virt_addr dataBaseAddress);

} // namespace cafe::hle
