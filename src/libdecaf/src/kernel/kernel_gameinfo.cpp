#include "decaf_game.h"
#include "kernel_filesystem.h"
#include "kernel_gameinfo.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <vector>

namespace kernel
{

static uint64_t
xmlReadUnsigned64(const pugi::xml_node &node)
{
   auto type = std::string { node.attribute("type").as_string() };
   auto length = node.attribute("length").as_uint();
   auto value = node.child_value();

   if (type == "hexBinary") {
      decaf_check(length <= 8);
      return std::stoull(value, 0, 16);
   } else if (type == "unsignedInt") {
      return std::stoull(value);
   }

   decaf_abort(fmt::format("Unexpected type {} for xmlReadUnsigned", type));
}

static unsigned
xmlReadUnsigned(const pugi::xml_node &node)
{
   auto type = std::string { node.attribute("type").as_string() };
   auto length = node.attribute("length").as_uint();
   auto value = node.child_value();

   if (type == "hexBinary") {
      decaf_check(length <= 8);
      return std::stoul(value, 0, 16);
   } else if (type == "unsignedInt") {
      return std::stoul(value);
   }

   decaf_abort(fmt::format("Unexpected type {} for xmlReadUnsigned", type));
}

static std::string
xmlReadString(const pugi::xml_node &node)
{
   return node.child_value();
}

bool
loadAppXML(const char *path,
           decaf::AppXML &app)
{
   auto fs = getFileSystem();
   auto result = fs->openFile(path, fs::File::Read);

   if (!result) {
      gLog->warn("Could not open {}, using default values", path);
      return false;
   }

   // Read whole file into buffer
   auto fh = result.value();
   auto size = fh->size();
   auto buffer = std::vector<uint8_t>(size);
   fh->read(buffer.data(), size, 1);
   fh->close();

   // Parse app.xml
   pugi::xml_document doc;
   auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!parseResult) {
      gLog->error("Error parsing {}", path);
      return false;
   }

   // Read values
   auto node = doc.child("app");
   app.version = xmlReadUnsigned(node.child("version"));
   app.os_version = xmlReadUnsigned64(node.child("os_version"));
   app.title_id = xmlReadUnsigned64(node.child("title_id"));
   app.title_version = xmlReadUnsigned(node.child("title_version"));
   app.sdk_version = xmlReadUnsigned(node.child("sdk_version"));
   app.app_type = xmlReadUnsigned(node.child("app_type"));
   app.group_id = xmlReadUnsigned(node.child("group_id"));

   return true;
}

bool
loadCosXML(const char *path,
           decaf::CosXML &cos)
{
   auto fs = getFileSystem();
   auto result = fs->openFile(path, fs::File::Read);

   if (!result) {
      gLog->warn("Could not open {}, using default values", path);
      return false;
   }

   // Read whole file into buffer
   auto fh = result.value();
   auto size = fh->size();
   auto buffer = std::vector<uint8_t>(size);
   fh->read(buffer.data(), size, 1);
   fh->close();

   // Parse app.xml
   pugi::xml_document doc;
   auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!parseResult) {
      gLog->error("Error parsing {}", path);
      return false;
   }

   // Read values
   auto node = doc.child("app");
   cos.version = xmlReadUnsigned(node.child("version"));
   cos.cmdFlags = xmlReadUnsigned(node.child("cmdFlags"));
   cos.argstr = xmlReadString(node.child("argstr"));
   cos.avail_size = xmlReadUnsigned(node.child("avail_size"));
   cos.codegen_size = xmlReadUnsigned(node.child("codegen_size"));
   cos.codegen_core = xmlReadUnsigned(node.child("codegen_core"));
   cos.max_size = xmlReadUnsigned(node.child("max_size"));
   cos.max_codesize = xmlReadUnsigned(node.child("max_codesize"));
   cos.default_stack0_size = xmlReadUnsigned(node.child("default_stack0_size"));
   cos.default_stack1_size = xmlReadUnsigned(node.child("default_stack1_size"));
   cos.default_stack2_size = xmlReadUnsigned(node.child("default_stack2_size"));
   cos.exception_stack0_size = xmlReadUnsigned(node.child("exception_stack0_size"));
   cos.exception_stack1_size = xmlReadUnsigned(node.child("exception_stack1_size"));
   cos.exception_stack2_size = xmlReadUnsigned(node.child("exception_stack2_size"));

   for (auto child : node.child("permissions").children()) {
      auto group = xmlReadUnsigned(child.child("group"));
      auto mask = xmlReadUnsigned64(child.child("mask"));

      switch (group) {
      case decaf::CosXML::FS:
         cos.permission_fs = mask;
         break;
      case decaf::CosXML::MCP:
         cos.permission_mcp = mask;
         break;
      }
   }

   return true;
}

static const char *
getLanguageSuffix(decaf::Language language)
{
   switch (language) {
   case decaf::Language::Japanese:
      return "ja";
   case decaf::Language::English:
      return "en";
   case decaf::Language::French:
      return "fr";
   case decaf::Language::German:
      return "de";
   case decaf::Language::Italian:
      return "it";
   case decaf::Language::Spanish:
      return "es";
   case decaf::Language::Chinese:
      return "zhs";
   case decaf::Language::Korean:
      return "ko";
   case decaf::Language::Dutch:
      return "nl";
   case decaf::Language::Portugese:
      return "pt";
   case decaf::Language::Russian:
      return "ru";
   case decaf::Language::Taiwanese:
      return "zht";
   default:
      decaf_abort("Something went very wrong.");
   }
}

bool
loadMetaXML(const char *path,
            decaf::MetaXML &meta)
{
   auto fs = getFileSystem();
   auto result = fs->openFile(path, fs::File::Read);

   if (!result) {
      gLog->warn("Could not open {}, using default values", path);
      return false;
   }

   // Read whole file into buffer
   auto fh = result.value();
   auto size = fh->size();
   auto buffer = std::vector<uint8_t>(size);
   fh->read(buffer.data(), size, 1);
   fh->close();

   // Parse app.xml
   pugi::xml_document doc;
   auto parseResult = doc.load_buffer_inplace(buffer.data(), buffer.size());

   if (!parseResult) {
      gLog->error("Error parsing {}", path);
      return false;
   }

   // Read values
   auto menu = doc.child("menu");
   meta.version = xmlReadUnsigned(menu.child("version"));
   meta.product_code = xmlReadString(menu.child("product_code"));
   meta.os_version = xmlReadUnsigned64(menu.child("os_version"));
   meta.title_version = xmlReadUnsigned(menu.child("title_version"));
   meta.title_id = xmlReadUnsigned64(menu.child("title_id"));
   meta.common_save_size = xmlReadUnsigned64(menu.child("common_save_size"));
   meta.account_save_size = xmlReadUnsigned64(menu.child("account_save_size"));
   meta.common_boss_size = xmlReadUnsigned64(menu.child("common_boss_size"));
   meta.account_boss_size = xmlReadUnsigned64(menu.child("account_boss_size"));
   meta.olv_accesskey = xmlReadUnsigned(menu.child("olv_accesskey"));
   meta.region = xmlReadUnsigned(menu.child("region"));

   // Read longname_{}
   for (auto id = 0u; id < decaf::Language::Max; ++id) {
      auto suffix = getLanguageSuffix(static_cast<decaf::Language>(id));
      auto key = fmt::format("longname_{}", suffix);
      meta.longnames[id] = xmlReadString(menu.child(key.c_str()));
   }

   // Read shortname_{}
   for (auto id = 0u; id < decaf::Language::Max; ++id) {
      auto suffix = getLanguageSuffix(static_cast<decaf::Language>(id));
      auto key = fmt::format("shortname_{}", suffix);
      meta.shortnames[id] = xmlReadString(menu.child(key.c_str()));
   }

   // Read publisher_{}
   for (auto id = 0u; id < decaf::Language::Max; ++id) {
      auto suffix = getLanguageSuffix(static_cast<decaf::Language>(id));
      auto key = fmt::format("publisher_{}", suffix);
      meta.publishers[id] = xmlReadString(menu.child(key.c_str()));
   }

   return true;
}

bool
loadGameInfo(decaf::GameInfo &info)
{
   if (!loadAppXML("/vol/code/app.xml", info.app)) {
      // Fill with some default values
      info.app.version = 1u;
      info.app.os_version = 0x000500101000400Aull;
      info.app.title_id = 0x00050000BADF000Dull;
      info.app.title_version = 1u;
      info.app.sdk_version = 21213u;
      info.app.app_type = 0x0800001Bu;
      info.app.group_id = 0u;
   }

   if (!loadCosXML("/vol/code/cos.xml", info.cos)) {
      // Fill with some default values
      info.cos.version = 1u;
      info.cos.cmdFlags = 0u;
      info.cos.argstr = {};
      info.cos.avail_size = 0u;
      info.cos.codegen_size = 0u;
      info.cos.codegen_core = 1u;
      info.cos.max_size = 0x40000000u;
      info.cos.max_codesize = 0x0E000000u;
      info.cos.default_stack0_size = 0u;
      info.cos.default_stack1_size = 0u;
      info.cos.default_stack2_size = 0u;
      info.cos.exception_stack0_size = 0x1000u;
      info.cos.exception_stack1_size = 0x1000u;
      info.cos.exception_stack2_size = 0x1000u;
      info.cos.permission_fs = decaf::CosXML::SdCardWrite;
      info.cos.permission_mcp = decaf::CosXML::AddOnContent;
   }

   if (!loadMetaXML("/vol/meta/meta.xml", info.meta)) {
      info.meta.version = 1u;
      info.meta.product_code = { };
      info.meta.os_version = info.app.os_version;
      info.meta.title_version = 1u;
      info.meta.title_id = info.app.title_id;
      info.meta.common_save_size = 0u;
      info.meta.account_save_size = 0x40000u;
      info.meta.common_boss_size = 0u;
      info.meta.account_boss_size = 0u;
      info.meta.olv_accesskey = 1337u;
      info.meta.region = 1u;
   }

   return true;
}

} // namespace kernel
