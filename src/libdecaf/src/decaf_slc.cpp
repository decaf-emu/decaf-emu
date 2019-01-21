#include "decaf_config.h"
#include "decaf_slc.h"

#include "ios/auxil/ios_auxil_enum.h"

#include <common/decaf_assert.h>
#include <common/platform_dir.h>
#include <common/strutils.h>
#include <filesystem>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <vector>

using ios::auxil::UCDataType;

namespace decaf::internal
{

struct InitialiseUCSysConfig
{
   const char *name;
   UCDataType dataType;
   const char *defaultValue = nullptr;
   uint32_t length = 0;
   uint32_t access = 777;
};

static InitialiseUCSysConfig
DefaultCafe[] = {
   { "cafe",                UCDataType::Complex              },
   { "cafe/version",        UCDataType::UnsignedShort, "5"   },
   { "cafe/language",       UCDataType::UnsignedInt,   "1"   },
   { "cafe/cntry_reg",      UCDataType::UnsignedInt,   "110" },
   { "cafe/eula_agree",     UCDataType::UnsignedByte,  "0"   },
   { "cafe/eula_version",   UCDataType::UnsignedInt,   "100" },
   { "cafe/initial_launch", UCDataType::UnsignedByte,  "2"   },
   { "cafe/eco",            UCDataType::UnsignedByte,  "0"   },
   { "cafe/fast_boot",      UCDataType::UnsignedByte,  "0"   },
};

constexpr InitialiseUCSysConfig
DefaultCaffeine[] = {
   { "caffeine",                  UCDataType::Complex },
   { "caffeine/version",          UCDataType::UnsignedShort, "3"          },
   { "caffeine/enable",           UCDataType::UnsignedByte,  "1"          },
   { "caffeine/ad_enable",        UCDataType::UnsignedByte,  "1"          },
   { "caffeine/push_enable",      UCDataType::UnsignedByte,  "1"          },
   { "caffeine/push_time_slot",   UCDataType::UnsignedInt,   "2132803072" },
   { "caffeine/push_interval",    UCDataType::UnsignedShort, "420"        },
   { "caffeine/drcled_enable",    UCDataType::UnsignedByte,  "1"          },
   { "caffeine/push_capabilty",   UCDataType::UnsignedShort, "65535"      },
   { "caffeine/invisible_titles", UCDataType::HexBinary,     nullptr, 512 },
};

constexpr InitialiseUCSysConfig
DefaultParent[] = {
   { "parent",                      UCDataType::Complex },
   { "parent/version",              UCDataType::UnsignedShort, "10"          },
   { "parent/enable",               UCDataType::UnsignedByte,  "0"           },
   { "parent/pin_code",             UCDataType::String,        nullptr, 5    },
   { "parent/sec_question",         UCDataType::UnsignedByte,  "0"           },
   { "parent/sec_answer",           UCDataType::String,        nullptr, 257  },
   { "parent/custom_sec_question",  UCDataType::String,        nullptr, 205  },
   { "parent/email_address",        UCDataType::String,        nullptr, 1025 },
   { "parent/permit_delete_all",    UCDataType::UnsignedByte,  "0"           },
   { "parent/rating_organization",  UCDataType::UnsignedInt,   "7"           },
};

constexpr InitialiseUCSysConfig
DefaultSpotpass[] = {
   { "spotpass",                       UCDataType::Complex           },
   { "spotpass/version",               UCDataType::UnsignedByte, "2" },
   { "spotpass/enable",                UCDataType::UnsignedByte, "1" },
   { "spotpass/auto_dl_app",           UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info1",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info2",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info3",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info4",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info5",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info6",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info7",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info8",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info9",  UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info10", UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info11", UCDataType::UnsignedByte, "1" },
   { "spotpass/upload_console_info12", UCDataType::UnsignedByte, "1" },
};

constexpr InitialiseUCSysConfig DefaultParentalAccount[] = {
   { "p_acct{}",                           UCDataType::Complex             },
   { "p_acct{}/version",                   UCDataType::UnsignedShort, "10" },
   { "p_acct{}/game_rating",               UCDataType::UnsignedInt,   "18" },
   { "p_acct{}/eshop_purchase",            UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/friend_reg",                UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/acct_modify",               UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/data_manage",               UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/int_setting",               UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/country_setting",           UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/sys_init",                  UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/int_browser",               UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/int_movie",                 UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/net_communication_on_game", UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/network_launcher",          UCDataType::UnsignedByte,  "0"  },
   { "p_acct{}/entertainment_launcher",    UCDataType::UnsignedByte,  "0"  },
};

static void
writeDefaultConfig(pugi::xml_node &doc,
                   const InitialiseUCSysConfig *first,
                   const InitialiseUCSysConfig *last,
                   int formatArg = 0)
{
   auto formattedName = std::string { };
   for (auto item = first; item != last; ++item) {
      auto parent = doc;
      auto name = std::string_view { item->name };

      if (name.find('{') != std::string::npos) {
         formattedName = fmt::format(name.data(), formatArg);
         name = formattedName;
      }

      if (auto pos = name.find_last_of("/"); pos != std::string_view::npos) {
         auto parentPath = std::string { name.substr(0, pos) };
         name = name.substr(pos + 1);

         parent = doc.first_element_by_path(parentPath.c_str());
         decaf_check(parent);
      }

      auto node = parent.append_child(name.data());

      if (item->defaultValue) {
         node.text() = item->defaultValue;
      } else if (item->dataType == UCDataType::HexBinary) {
         node.text() = std::string(item->length * 2, '0').c_str();
      }

      switch (item->dataType) {
      case UCDataType::Complex:
         node.append_attribute("type") = "complex";
         break;
      case UCDataType::UnsignedByte:
         node.append_attribute("type") = "unsignedByte";
         node.append_attribute("length") = "1";
         break;
      case UCDataType::UnsignedShort:
         node.append_attribute("type") = "unsignedShort";
         node.append_attribute("length") = "2";
         break;
      case UCDataType::UnsignedInt:
         node.append_attribute("type") = "unsignedInt";
         node.append_attribute("length") = "4";
         break;
      case UCDataType::SignedInt:
         node.append_attribute("type") = "signedInt";
         node.append_attribute("length") = "4";
         break;
      case UCDataType::HexBinary:
         node.append_attribute("type") = "hexBinary";
         node.append_attribute("length") = item->length;
         break;
      case UCDataType::String:
         node.append_attribute("type") = "string";
         node.append_attribute("length") = item->length;
         break;
      default:
         decaf_abort("Unimplemented UCDataType");
      }

      if (item->access) {
         node.append_attribute("access") = item->access;
      }
   }
}

static void
writeDefaultConfig(const std::filesystem::path &path,
                   const InitialiseUCSysConfig *first,
                   const InitialiseUCSysConfig *last,
                   int formatArg = 0)
{
   if (std::filesystem::exists(path)) {
      return;
   }

   auto doc = pugi::xml_document { };
   auto decl = doc.prepend_child(pugi::node_declaration);
   decl.append_attribute("version") = "1.0";
   decl.append_attribute("encoding") = "utf-8";

   writeDefaultConfig(doc, first, last, formatArg);

   doc.save_file(path.string().c_str(),
                 "  ", 1,
                 pugi::encoding_utf8);
}

static void
applyRegionSettings()
{
   auto language = DefaultCafe[2].defaultValue;
   auto country = DefaultCafe[3].defaultValue;

   switch (config()->system.region) {
   case  SystemRegion::Japan:
      language = "0"; // Japanese
      country = "3"; // Japan
      break;
   case SystemRegion::USA:
      language = "1"; // English
      country = "49"; // United States
      break;
   case SystemRegion::Europe:
      language = "1"; // English
      country = "110"; // United Kingdom
      break;
   case SystemRegion::China:
      language = "6"; // Chinese
      country = "160"; // China
      break;
   case SystemRegion::Korea:
      language = "7"; // Korean
      country = "136"; // Best Korea
      break;
   case SystemRegion::Taiwan:
      language = "11"; // Taiwanese
      country = "128"; // Taiwan
      break;
   }

   DefaultCafe[2].defaultValue = language;
   DefaultCafe[3].defaultValue = country;
}

void
initialiseSlc(std::string_view path)
{
   // Ensure slc/proc/prefs exists
   auto prefsPath = std::filesystem::path { path } / "proc" / "prefs";
   std::filesystem::create_directories(prefsPath);

   // Apply decaf region config to XML settings before we write them
   applyRegionSettings();

   // Write some default config settings if they don't already exist
   writeDefaultConfig(prefsPath / "cafe.xml",
                      std::begin(DefaultCafe),
                      std::end(DefaultCafe));

   writeDefaultConfig(prefsPath / "caffeine.xml",
                      std::begin(DefaultCaffeine),
                      std::end(DefaultCaffeine));

   writeDefaultConfig(prefsPath / "parent.xml",
                      std::begin(DefaultParent),
                      std::end(DefaultParent));

   writeDefaultConfig(prefsPath / "spotpass.xml",
                      std::begin(DefaultSpotpass),
                      std::end(DefaultSpotpass));

   for (auto i = 1; i <= 12; ++i) {
      writeDefaultConfig(prefsPath / fmt::format("p_acct{}.xml", i),
                         std::begin(DefaultParentalAccount),
                         std::end(DefaultParentalAccount),
                         i);
   }
}

} // namespace decaf::internal
