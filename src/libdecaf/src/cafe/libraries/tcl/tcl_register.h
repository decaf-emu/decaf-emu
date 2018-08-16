#pragma once
#include "tcl_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

TCLStatus
TCLReadRegister(TCLRegisterID id,
                virt_ptr<uint32_t> outValue);

TCLStatus
TCLWriteRegister(TCLRegisterID id,
                 uint32_t value);

} // namespace cafe::tcl
