#pragma once
#include "cafe_loader_rpl.h"
#include <libcpu/be2_struct.h>

namespace cafe::loader::internal
{

int
ELFFILE_ValidateAndPrepareMinELF(virt_ptr<void> chunkBuffer,
                                 size_t chunkSize,
                                 virt_ptr<rpl::Header> *outHeader,
                                 virt_ptr<rpl::SectionHeader> *outSectionHeaders,
                                 uint32_t *outShEntSize,
                                 uint32_t *outPhEntSize);

} // namespace cafe::loader::internal
