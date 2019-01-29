#include "ios_acp.h"
#include "ios_acp_metaxml.h"

#include "nn/acp/nn_acp_result.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"

#include <common/strutils.h>
#include <fmt/format.h>
#include <pugixml.hpp>

using namespace ios::fs;
using namespace ios::kernel;
using namespace nn::acp;

namespace ios::acp::internal
{

static nn::Result
getU32(pugi::xml_node &node,
       const char *name,
       phys_ptr<uint32_t> outValue)
{
   auto child = node.child(name);
   if (!child) {
      return ResultXmlNodeNotFound;
   }

   *outValue = static_cast<uint32_t>(strtoul(child.text().get(), nullptr, 10));
   return ResultSuccess;
}

static nn::Result
getHex32(pugi::xml_node &node,
         const char *name,
         phys_ptr<uint32_t> outValue)
{
   auto child = node.child(name);
   if (!child) {
      return ResultXmlNodeNotFound;
   }

   *outValue = static_cast<uint32_t>(strtoul(child.text().get(), nullptr, 16));
   return ResultSuccess;
}

static nn::Result
getHex64(pugi::xml_node &node,
         const char *name,
         phys_ptr<uint64_t> outValue)
{
   auto child = node.child(name);
   if (!child) {
      return ResultXmlNodeNotFound;
   }

   *outValue = static_cast<uint64_t>(strtoull(child.text().get(), nullptr, 16));
   return ResultSuccess;
}

template<uint32_t Size>
static nn::Result
getString(pugi::xml_node &node,
          const char *name,
          be2_array<char, Size> &buffer)
{
   auto child = node.child(name);
   if (!child) {
      return ResultXmlNodeNotFound;
   }

   if (!child.text()) {
      buffer[0] = char { 0 };
      return ResultSuccess;
   }

   string_copy(phys_addrof(buffer).get(), child.text().get(), Size);
   return ResultSuccess;
}

nn::Result
loadMetaXMLFromPath(std::string_view path,
                    phys_ptr<nn::acp::ACPMetaXml> metaXml)
{
   auto xmlSize = uint32_t { 0 };
   auto xmlData = phys_ptr<uint8_t> { nullptr };
   auto fsaStatus = FSAReadFileIntoCrossProcessHeap(getFsaHandle(),
                                                    "/vol/meta/meta.xml",
                                                    &xmlSize, &xmlData);
   if (fsaStatus < FSAStatus::OK) {
      return static_cast<nn::Result>(fsaStatus);
   }

   auto doc = pugi::xml_document { };
   auto parseResult = doc.load_buffer(xmlData.get(), xmlSize);
   IOS_HeapFree(CrossProcessHeapId, xmlData);

   if (!parseResult) {
      return ResultXmlRootNodeNotFound;
   }

   auto menu = doc.child("menu");
   if (!menu) {
      return ResultXmlRootNodeNotFound;
   }

   auto result = getU32(menu, "version", phys_addrof(metaXml->version));
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "product_code", metaXml->product_code);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "content_platform", metaXml->content_platform);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "company_code", metaXml->company_code);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "mastering_date", metaXml->mastering_date);
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "logo_type", phys_addrof(metaXml->logo_type));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "app_launch_type", phys_addrof(metaXml->app_launch_type));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "invisible_flag", phys_addrof(metaXml->invisible_flag));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "no_managed_flag", phys_addrof(metaXml->no_managed_flag));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "no_event_log", phys_addrof(metaXml->no_event_log));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "no_icon_database", phys_addrof(metaXml->no_icon_database));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "launching_flag", phys_addrof(metaXml->launching_flag));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "install_flag", phys_addrof(metaXml->install_flag));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "closing_msg", phys_addrof(metaXml->closing_msg));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "title_version", phys_addrof(metaXml->title_version));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "title_id", phys_addrof(metaXml->title_id));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "group_id", phys_addrof(metaXml->group_id));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "boss_id", phys_addrof(metaXml->boss_id));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "os_version", phys_addrof(metaXml->os_version));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "app_size", phys_addrof(metaXml->app_size));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "common_save_size", phys_addrof(metaXml->common_save_size));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "account_save_size", phys_addrof(metaXml->account_save_size));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "common_boss_size", phys_addrof(metaXml->common_boss_size));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "account_boss_size", phys_addrof(metaXml->account_boss_size));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "save_no_rollback", phys_addrof(metaXml->save_no_rollback));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "bg_daemon_enable", phys_addrof(metaXml->bg_daemon_enable));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "join_game_id", phys_addrof(metaXml->join_game_id));
   if (result.failed()) {
      return result;
   }

   result = getHex64(menu, "join_game_mode_mask", phys_addrof(metaXml->join_game_mode_mask));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "olv_accesskey", phys_addrof(metaXml->olv_accesskey));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "wood_tin", phys_addrof(metaXml->wood_tin));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "e_manual", phys_addrof(metaXml->e_manual));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "e_manual_version", phys_addrof(metaXml->e_manual_version));
   if (result.failed()) {
      return result;
   }

   result = getHex32(menu, "region", phys_addrof(metaXml->region));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_cero", phys_addrof(metaXml->pc_cero));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_esrb", phys_addrof(metaXml->pc_esrb));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_bbfc", phys_addrof(metaXml->pc_bbfc));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_usk", phys_addrof(metaXml->pc_usk));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_pegi_gen", phys_addrof(metaXml->pc_pegi_gen));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_pegi_fin", phys_addrof(metaXml->pc_pegi_fin));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_pegi_prt", phys_addrof(metaXml->pc_pegi_prt));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_pegi_bbfc", phys_addrof(metaXml->pc_pegi_bbfc));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_cob", phys_addrof(metaXml->pc_cob));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_grb", phys_addrof(metaXml->pc_grb));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_cgsrr", phys_addrof(metaXml->pc_cgsrr));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_oflc", phys_addrof(metaXml->pc_oflc));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_reserved0", phys_addrof(metaXml->pc_reserved0));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_reserved1", phys_addrof(metaXml->pc_reserved1));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_reserved2", phys_addrof(metaXml->pc_reserved2));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "pc_reserved3", phys_addrof(metaXml->pc_reserved3));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_nunchaku", phys_addrof(metaXml->ext_dev_nunchaku));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_classic", phys_addrof(metaXml->ext_dev_classic));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_urcc", phys_addrof(metaXml->ext_dev_urcc));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_board", phys_addrof(metaXml->ext_dev_board));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_usb_keyboard", phys_addrof(metaXml->ext_dev_usb_keyboard));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "ext_dev_etc", phys_addrof(metaXml->ext_dev_etc));
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "ext_dev_etc_name", metaXml->ext_dev_etc_name);
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "eula_version", phys_addrof(metaXml->eula_version));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "drc_use", phys_addrof(metaXml->drc_use));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "network_use", phys_addrof(metaXml->network_use));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "online_account_use", phys_addrof(metaXml->online_account_use));
   if (result.failed()) {
      return result;
   }

   result = getU32(menu, "direct_boot", phys_addrof(metaXml->direct_boot));
   if (result.failed()) {
      return result;
   }

   for (auto i = 0u; i < metaXml->reserved_flags.size(); ++i) {
      result = getHex32(menu, fmt::format("reserved_flag{}", i).c_str(),
                        phys_addrof(metaXml->reserved_flags[i]));
      if (result.failed()) {
         return result;
      }
   }

   result = getString(menu, "longname_ja", metaXml->longname_ja);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_en", metaXml->longname_en);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_fr", metaXml->longname_fr);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_de", metaXml->longname_de);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_it", metaXml->longname_it);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_es", metaXml->longname_es);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_zhs", metaXml->longname_zhs);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_ko", metaXml->longname_ko);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_nl", metaXml->longname_nl);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_pt", metaXml->longname_pt);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_ru", metaXml->longname_ru);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "longname_zht", metaXml->longname_zht);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_ja", metaXml->shortname_ja);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_en", metaXml->shortname_en);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_fr", metaXml->shortname_fr);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_de", metaXml->shortname_de);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_it", metaXml->shortname_it);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_es", metaXml->shortname_es);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_zhs", metaXml->shortname_zhs);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_ko", metaXml->shortname_ko);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_nl", metaXml->shortname_nl);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_pt", metaXml->shortname_pt);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_ru", metaXml->shortname_ru);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "shortname_zht", metaXml->shortname_zht);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_ja", metaXml->publisher_ja);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_en", metaXml->publisher_en);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_fr", metaXml->publisher_fr);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_de", metaXml->publisher_de);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_it", metaXml->publisher_it);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_es", metaXml->publisher_es);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_zhs", metaXml->publisher_zhs);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_ko", metaXml->publisher_ko);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_nl", metaXml->publisher_nl);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_pt", metaXml->publisher_pt);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_ru", metaXml->publisher_ru);
   if (result.failed()) {
      return result;
   }

   result = getString(menu, "publisher_zht", metaXml->publisher_zht);
   if (result.failed()) {
      return result;
   }

   for (auto i = 0u; i < metaXml->add_on_unique_ids.size(); ++i) {
      result = getHex32(menu, fmt::format("add_on_unique_id{}", i).c_str(),
                        phys_addrof(metaXml->add_on_unique_ids[i]));
      if (result.failed()) {
         return result;
      }
   }

   return ResultSuccess;
}

} // namespace ios::acp::internal
