#include "ios_auxil_usr_cfg_device.h"
#include "ios_auxil_usr_cfg_fs.h"
#include "ios_auxil_usr_cfg_service_thread.h"

#include <array>
#include <common/strutils.h>
#include <fmt/format.h>
#include <pugixml.hpp>
#include <sstream>
#include <string_view>

namespace ios::auxil::internal
{

struct Key
{
   enum Storage
   {
      Default,
      Slc,
      RamDisk,
   };

   Storage storage;
   std::string fullname;
   std::string path;
   std::string name;
   std::string rootKey;
};

static std::array<const char *, 66>
sValidRootKeys =
{
   "AVMCfg",
   "DRCCfg",
   "NETCfg0",
   "NETCfg1",
   "NETCfg2",
   "NETCfg3",
   "NETCfg4",
   "NETCfg5",
   "NETCmn",
   "audio",
   "avm_cdc",
   "avmflg",
   "avmStat",
   "btStd",
   "cafe",
   "ccr",
   "ccr_all",
   "coppa",
   "cos",
   "dc_state",
   "im_cfg",
   "nn",
   "nn_ram",
   "hbprefs",
   "p_acct1",
   "p_acct10",
   "p_acct11",
   "p_acct12",
   "p_acct2",
   "p_acct3",
   "p_acct4",
   "p_acct5",
   "p_acct6",
   "p_acct7",
   "p_acct8",
   "p_acct9",
   "parent",
   "PCFSvTCP",
   "console",
   "rmtCfg",
   "rootflg",
   "spotpass",
   "tvecfg",
   "tveEDID",
   "tveHdmi",
   "uvdflag",
   "wii_acct",
   "ums",
   "testProc",
   "clipbd",
   "hdmiEDID",
   "caffeine",
   "DRCDev",
   "hai_sys",
   "s_acct01",
   "s_acct02",
   "s_acct03",
   "s_acct04",
   "s_acct05",
   "s_acct06",
   "s_acct07",
   "s_acct08",
   "s_acct09",
   "s_acct10",
   "s_acct11",
   "s_acct12",
};


static bool
isValidRootKey(std::string_view key)
{
   for (auto validKey : sValidRootKeys) {
      if (key.compare(validKey) == 0) {
         return true;
      }
   }

   return false;
}


static Key
getKeyFromName(std::string name)
{
   Key key;
   key.fullname = name;

   if (begins_with(name, "slc:")) {
      key.storage = Key::Slc;
      name.erase(name.begin(), name.begin() + 4);
   } else if (begins_with(name, "ram:")) {
      key.storage = Key::RamDisk;
      name.erase(name.begin(), name.begin() + 4);
   } else {
      decaf_check(name.find_first_of(':') == std::string::npos);
      key.storage = Key::Default;
   }

   auto pos = name.find_first_of('.');
   if (pos == std::string::npos) {
      key.rootKey = name;
   } else {
      key.rootKey = name.substr(0, pos);
   }

   pos = name.find_last_of('.');
   if (pos == std::string::npos) {
      key.name = name;
   } else {
      key.path = name.substr(0, pos);
      key.name = name.substr(pos + 1);
   }

   return key;
}


static const char *
getDataTypeName(UCDataType type)
{
   switch (type) {
   case UCDataType::UnsignedByte:
      return "unsignedByte";
   case UCDataType::UnsignedShort:
      return "unsignedShort";
   case UCDataType::UnsignedInt:
      return "unsignedInt";
   case UCDataType::SignedInt:
      return "signedInt";
   case UCDataType::Float:
      return "float";
   case UCDataType::String:
      return "string";
   case UCDataType::HexBinary:
      return "hexBinary";
   case UCDataType::Complex:
      return "complex";
   default:
      return nullptr;
   }
}


static UCDataType
getDataTypeByName(const char *name)
{
   if (strcmp(name, "unsignedByte") == 0) {
      return UCDataType::UnsignedByte;
   } else if (strcmp(name, "unsignedShort") == 0) {
      return UCDataType::UnsignedShort;
   } else if (strcmp(name, "unsignedInt") == 0) {
      return UCDataType::UnsignedInt;
   } else if (strcmp(name, "signedInt") == 0) {
      return UCDataType::SignedInt;
   } else if (strcmp(name, "float") == 0) {
      return UCDataType::Float;
   } else if (strcmp(name, "string") == 0) {
      return UCDataType::String;
   } else if (strcmp(name, "hexBinary") == 0) {
      return UCDataType::HexBinary;
   } else if (strcmp(name, "complex") == 0) {
      return UCDataType::Complex;
   } else {
      return UCDataType::Invalid;
   }
}


static uint32_t
getAccessFromString(const char *str)
{
   auto access = 0u;

   if (!str || strlen(str) != 3) {
      return 0;
   }

   for (auto i = 0u; i < 3; ++i) {
      auto val = str[i] - '0';
      if (val < 0 || val > 7) {
         return 0;
      }

      access |= val << (4 * i);
   }

   return access;
}


static std::string
getAccessString(uint32_t access)
{
   return fmt::format("{}{}{}", (access >> 8) & 0xf, (access >> 4) & 0xf, access & 0xf);
}


static std::string
to_string(uint8_t *data,
          size_t size)
{
   fmt::MemoryWriter out;

   for (auto i = 0u; i < size; ++i) {
      out.write("{:02X}", data[i]);
   }

   return out.str();
}


void
UCDevice::setCloseRequest(phys_ptr<kernel::ResourceRequest> closeRequest)
{
   mCloseRequest = closeRequest;
}


void
UCDevice::incrementRefCount()
{
   mRefCount++;
}

void
UCDevice::decrementRefCount()
{
   mRefCount--;

   if (mRefCount == 0) {
      if (mCloseRequest) {
         kernel::IOS_ResourceReply(mCloseRequest, Error::OK);
      }

      mCloseRequest = nullptr;
      destroyUCDevice(this);
   }
}

UCError
UCDevice::readSysConfig(uint32_t numVecIn,
                        phys_ptr<IoctlVec> vecs)
{
   auto request = phys_ptr<UCReadSysConfigRequest> { vecs[0].paddr };
   decaf_check(request->size == sizeof(UCSysConfig));

   for (auto i = 0u; i < request->count; ++i) {
      auto &setting = request->settings[i];
      auto key = getKeyFromName(phys_addrof(setting.name).getRawPointer());
      auto path = std::string {};

      // Calculate the path
      if (key.storage == Key::Slc) {
         path = "/vol/sys/proc_slc/prefs/";
      } else if (key.storage == Key::RamDisk) {
         path = "/vol/sys/proc_ram/prefs/";
      } else {
         path = "/vol/sys/proc/prefs/";
      }

      // Verify valid root key
      if (!isValidRootKey(key.rootKey)) {
         // TODO: Not sure if correct error code
         setting.error = UCError::FileSysName;
         continue;
      }

      path += key.rootKey + ".xml";

      // Read the file data
      uint32_t fileSize;
      phys_ptr<uint8_t> fileBuffer;

      auto error = UCReadConfigFile(path, &fileSize, &fileBuffer);
      if (error < UCError::OK) {
         setting.error = error;
         continue;
      }

      // Parse XML
      pugi::xml_document doc;
      auto parseResult = doc.load_buffer(fileBuffer.getRawPointer(), fileSize);
      UCFreeFileData(fileBuffer, fileSize);

      if (!parseResult) {
         setting.error = UCError::MalformedXML;
         continue;
      }

      auto nodePath = key.path;
      replace_all(nodePath, '.', '/');
      nodePath += '/';
      nodePath += key.name;

      auto node = doc.first_element_by_path(nodePath.c_str());
      if (!node) {
         setting.error = UCError::KeyNotFound;
         continue;
      }

      auto nodeType = getDataTypeByName(node.attribute("type").as_string());
      if (nodeType != setting.dataType) {
         setting.error = UCError::InvalidType;
         continue;
      }

      auto nodeLength = node.attribute("length").as_uint();
      if (setting.dataType != UCDataType::Complex && !setting.data) {
         setting.error = UCError::InvalidParam;
         continue;
      }

      auto nodeAccess = node.attribute("access");
      if (nodeAccess) {
         setting.access = getAccessFromString(nodeAccess.value());
      }

      auto settingData = phys_ptr<void>(vecs[i + 1].paddr);
      switch (setting.dataType) {
      case UCDataType::UnsignedByte:
         if (setting.dataSize >= 1) {
            *phys_cast<uint8_t>(settingData) = static_cast<uint8_t>(node.text().as_uint());
         } else {
            setting.error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::UnsignedShort:
         if (setting.dataSize >= 2) {
            *phys_cast<uint16_t>(settingData) = static_cast<uint16_t>(node.text().as_uint());
         } else {
            setting.error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::UnsignedInt:
         if (setting.dataSize >= 4) {
            *phys_cast<uint32_t>(settingData) = static_cast<uint32_t>(node.text().as_uint());
         } else {
            setting.error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::SignedInt:
         if (setting.dataSize >= 4) {
            *phys_cast<int32_t>(settingData) = static_cast<int32_t>(node.text().as_int());
         } else {
            setting.error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::Float:
         if (setting.dataSize >= 4) {
            *phys_cast<float>(settingData) = node.text().as_float();
         } else {
            setting.error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::String:
      {
         auto str = node.text().get();
         auto size = strlen(str);

         if (size < setting.dataSize) {
            std::memcpy(setting.data.getRawPointer(), str, size + 1);
            setting.dataSize = static_cast<uint32_t>(size + 1);
         } else {
            setting.error = UCError::StringTooLong;
            continue;
         }
      }
      case UCDataType::HexBinary:
      {
         auto src = node.text().get();
         auto size = strlen(src) / 2;

         if (size <= setting.dataSize) {
            auto dst = phys_cast<uint8_t>(settingData);

            for (auto i = 0u; i < size; ++i) {
               auto value = uint8_t { 0 };
               value |= src[i * 2 + 0] << 4;
               value |= src[i * 2 + 1];
               dst[i] = value;
            }

            setting.dataSize = static_cast<uint32_t>(size);
         } else {
            setting.error = UCError::StringTooLong;
            continue;
         }
         break;
      }
      case UCDataType::Complex:
         break;
      default:
         setting.error = UCError::InvalidType;
         continue;
      }

      setting.error = UCError::OK;
   }

   return UCError::OK;
}


UCError
UCDevice::writeSysConfig(uint32_t numVecIn,
                         phys_ptr<IoctlVec> vecs)
{
   auto request = phys_ptr<UCWriteSysConfigRequest> { vecs[0].paddr };
   decaf_check(request->size == sizeof(UCSysConfig));

   for (auto i = 0u; i < request->count; ++i) {
      auto &setting = request->settings[i];
      auto key = getKeyFromName(phys_addrof(setting.name).getRawPointer());
      auto path = std::string { };

      if (key.storage == Key::Slc) {
         path = "/vol/sys/proc_slc/prefs/";
      } else if (key.storage == Key::RamDisk) {
         path = "/vol/sys/proc_ram/prefs/";
      } else {
         path = "/vol/sys/proc/prefs/";
      }

      if (!isValidRootKey(key.rootKey.c_str())) {
         // TODO: Not sure if correct error code
         setting.error = UCError::FileSysName;
         continue;
      }

      pugi::xml_document doc;
      path += key.rootKey + ".xml";

      // Read the file data
      uint32_t fileSize;
      phys_ptr<uint8_t> fileBuffer;

      auto error = UCReadConfigFile(path, &fileSize, &fileBuffer);
      if (error >= UCError::OK) {
         auto parseResult = doc.load_buffer(fileBuffer.getRawPointer(), fileSize);
         UCFreeFileData(fileBuffer, fileSize);

         if (!parseResult) {
            setting.error = UCError::MalformedXML;
            continue;
         }
      }

      if (key.path.empty()) {
         auto node = doc.first_child();
         if (node) {
            if (key.name != node.name()) {
               setting.error = UCError::RootKeysDiffer;
               continue;
            }

            if (strcmp(node.attribute("type").as_string(), "complex")) {
               setting.error = UCError::RootKeysDiffer;
               continue;
            }
         } else {
            node = doc.append_child(key.name.c_str());
            node.append_attribute("type").set_value("complex");

            if (setting.access) {
               node.append_attribute("access");
            }
         }

         if (setting.access) {
            node.attribute("access").set_value(getAccessString(setting.access).c_str());
         }
      } else {
         auto nodePath = key.path;
         replace_all(nodePath, '.', '/');

         auto parent = doc.first_element_by_path(nodePath.c_str());
         if (!parent) {
            setting.error = UCError::KeyNotFound;
            continue;
         }

         auto parentType = parent.attribute("type");
         if (strcmp(parentType.as_string(), "complex")) {
            setting.error = UCError::KeyNotFound;
            continue;
         }

         auto node = parent.child(key.name.c_str());
         if (!node) {
            node = parent.append_child(key.name.c_str());
            node.append_attribute("type");

            if (setting.dataType != UCDataType::Complex) {
               node.append_attribute("length");
            }

            if (setting.access) {
               node.append_attribute("access");
            }
         }

         auto dataTypeName = getDataTypeName(setting.dataType);
         if (!dataTypeName) {
            setting.error = UCError::InvalidType;
            continue;
         }

         node.attribute("type").set_value(dataTypeName);

         if (setting.dataType != UCDataType::Complex) {
            node.attribute("length").set_value(setting.dataSize);
         }

         if (setting.access) {
            node.attribute("access").set_value(getAccessString(setting.access).c_str());
         }

         auto settingData = phys_ptr<void>(vecs[i + 1].paddr);
         switch (setting.dataType) {
         case UCDataType::UnsignedByte:
            if (setting.data) {
               node.text().set(*phys_cast<uint8_t>(settingData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::UnsignedShort:
            if (setting.data) {
               node.text().set(*phys_cast<uint16_t>(settingData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::UnsignedInt:
            if (setting.data) {
               node.text().set(*phys_cast<uint32_t>(settingData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::SignedInt:
            if (setting.data) {
               node.text().set(*phys_cast<int32_t>(settingData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::Float:
            if (setting.data) {
               node.text().set(*phys_cast<float>(settingData));
            } else {
               node.text().set(0.0f);
            }
            break;
         case UCDataType::String:
            if (setting.data) {
               // TODO: Check text format, maybe utf8/utf16 etc?
               node.text().set(phys_cast<char>(settingData).getRawPointer());
            } else {
               node.text().set("");
            }
            break;
         case UCDataType::HexBinary:
            if (setting.data) {
               node.text().set(to_string(phys_cast<uint8_t>(settingData).getRawPointer(), setting.dataSize).c_str());
            } else {
               std::string value(static_cast<size_t>(setting.dataSize * 2), '0');
               node.text().set(value.c_str());
            }
            break;
         default:
            setting.error = UCError::InvalidType;
            continue;
         }
      }

      // Save file
      std::stringstream ss;
      doc.save(ss, "  ", 1, pugi::encoding_utf8);

      auto xmlStr = ss.str();

      fileSize = xmlStr.size() + 1;
      fileBuffer = UCAllocFileData(fileSize);
      if (!fileBuffer) {
         setting.error = UCError::Alloc;
         continue;
      }

      error = UCWriteConfigFile(path, fileBuffer, fileSize);
      UCFreeFileData(fileBuffer, fileSize);
      if (error < UCError::OK) {
         setting.error = error;
         continue;
      }

      setting.error = UCError::OK;
   }

   return UCError::OK;
}

} // namespace ios::auxil::internal
