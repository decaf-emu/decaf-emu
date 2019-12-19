#include "cafe_ppc_interface_trace_host.h"

#include <common/log.h>
#include <fmt/format.h>
#include <libcpu/cpu_formatters.h>
#include <string_view>

namespace cafe::detail
{

inline uint32_t
readGpr(cpu::Core *core, int regIndex)
{
   if (regIndex <= 10) {
      return core->gpr[regIndex];
   } else {
      // Args come after the backchain from the caller (8 bytes).
      auto addr = core->gpr[1] + 8 + 4 * static_cast<uint32_t>(regIndex - 11);
      return *virt_cast<uint32_t *>(virt_addr { addr });
   }
}

void
invoke_trace_host_impl(cpu::Core *core, const char *name, bool is_member_function, const RuntimeParamInfo *params, size_t numParams)
{
   fmt::memory_buffer message;
   fmt::format_to(message, "{}(", name);

   if (is_member_function) {
      fmt::format_to(message, "this = {}, ", static_cast<virt_addr>(readGpr(core, 3)));
   }

   for (auto i = 0u; i < numParams; ++i) {
      auto &p = params[i];

      if (i > 0) {
         fmt::format_to(message, ", ");
      }

      switch (p.reg_type) {
      case RegisterType::Gpr32:
         if (p.is_string) {
            auto value = readGpr(core, p.reg_index);
            if (value) {
               fmt::format_to(message, "\"{}\"", virt_cast<const char *>(static_cast<virt_addr>(value)).get());
            } else {
               fmt::format_to(message, "{}", static_cast<virt_addr>(value));
            }
         } else if (p.is_pointer) {
            fmt::format_to(message, "{}", static_cast<virt_addr>(readGpr(core, p.reg_index)));
         } else  if (p.is_signed) {
            fmt::format_to(message, "{}", static_cast<int32_t>(readGpr(core, p.reg_index)));
         } else {
            fmt::format_to(message, "{}", readGpr(core, p.reg_index));
         }
         break;
      case RegisterType::Gpr64:
      {
         auto hi = static_cast<uint64_t>(readGpr(core, p.reg_index)) << 32;
         auto lo = static_cast<uint64_t>(readGpr(core, p.reg_index + 1));
         if (p.is_signed) {
            fmt::format_to(message, "{}", static_cast<int64_t>(hi | lo));
         } else {
            fmt::format_to(message, "{}", static_cast<uint64_t>(hi | lo));
         }
         break;
      }
      case RegisterType::Fpr:
         fmt::format_to(message, "{}", core->fpr[p.reg_index].paired0);
         break;
      case RegisterType::VarArgs:
         fmt::format_to(message, "...");
         break;
      case RegisterType::Void:
         break;
      }
   }

   fmt::format_to(message, ") from 0x{:08X}", core->lr);
   gLog->debug(std::string_view { message.data(), message.size() });
}

} // namespace cafe::detail
