#pragma once
#include "cafe_loader_basics.h"
#include "cafe_loader_rpl.h"
#include <libcpu/be2_struct.h>
#include <string_view>

namespace cafe::loader::internal
{

inline  std::string_view
LiResolveModuleName(std::string_view name)
{
   auto pos = name.find_last_of("\\/");
   if (pos != std::string_view::npos) {
      name = name.substr(pos + 1);
   }

   pos = name.find_first_of(".");
   if (pos != std::string_view::npos) {
      name = name.substr(0, pos);
   }

   return name;
}

inline void
LiResolveModuleName(virt_ptr<virt_ptr<char>> moduleName,
                    virt_ptr<uint32_t> moduleNameLen)
{
   auto name = std::string_view { moduleName->get(), *moduleNameLen };
   auto pos = name.find_last_of("\\/");
   if (pos != std::string_view::npos) {
      auto diff = static_cast<uint32_t>(pos + 1);
      name = name.substr(diff);
      *moduleName += diff;
      *moduleNameLen -= diff;
   }

   pos = name.find_first_of(".");
   if (pos != std::string_view::npos) {
      auto diff = static_cast<uint32_t>(name.size() - pos);
      *moduleNameLen -= diff;
   }

   (*moduleName)[*moduleNameLen] = char { 0 };
}

inline virt_ptr<rpl::SectionHeader>
getSectionHeader(virt_ptr<LOADED_RPL> rpl, uint32_t index)
{
   return virt_cast<rpl::SectionHeader *>(
      virt_cast<virt_addr>(rpl->sectionHeaderBuffer) +
      (index * rpl->elfHeader.shentsize));
}

} // namespace cafe::loader::internal
