#include "cafe/cafe_ppc_interface_varargs.h"
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

namespace internal
{

bool
formatStringV(virt_ptr<const char> fmt,
              virt_ptr<va_list> list,
              std::string &output);

int32_t
formatStringV(virt_ptr<char> buffer,
              uint32_t len,
              virt_ptr<const char> fmt,
              virt_ptr<va_list> list);

} // namespace internal

} // namespace cafe::coreinit
