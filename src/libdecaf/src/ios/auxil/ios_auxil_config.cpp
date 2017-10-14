#include "ios_auxil_config.h"

#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/ios_stackobject.h"

#include <array>
#include <common/strutils.h>
#include <pugixml.hpp>
#include <sstream>

namespace ios::auxil
{

using namespace ios::fs;
using namespace ios::kernel;

static std::array<FSAHandle, NumIosProcess>
sFsaHandles;

Error
openFsaHandle()
{
   auto error = IOS_GetCurrentProcessId();
   if (error < Error::OK) {
      return error;
   }
   auto pid = static_cast<ProcessId>(error);

   error = FSAOpen();
   if (error < Error::OK) {
      return error;
   }
   auto handle = static_cast<FSAHandle>(error);

   sFsaHandles[pid] = handle;
   return Error::OK;
}

Error
closeFsaHandle()
{
   auto error = IOS_GetCurrentProcessId();
   if (error < Error::OK) {
      return error;
   }
   auto pid = static_cast<ProcessId>(error);

   error = FSAClose(sFsaHandles[pid]);
   sFsaHandles[pid] = Error::Invalid;
   return error;
}

static FSAHandle
getFsaHandle()
{
   return sFsaHandles.at(IOS_GetCurrentProcessId());
}

static phys_ptr<uint8_t>
allocFileData(uint32_t size)
{
   auto ptr = IOS_HeapAllocAligned(CrossProcessHeapId, size, 0x40u);
   if (ptr) {
      std::memset(ptr.getRawPointer(), 0, size);
   }

   return phys_cast<uint8_t *>(ptr);
}

static void
freeFileData(phys_ptr<uint8_t> fileData,
             uint32_t size)
{
   IOS_HeapFree(CrossProcessHeapId, fileData);
}

static UCError
readFile(std::string_view filename,
         uint32_t *outBytesRead,
         phys_ptr<uint8_t> *outBuffer)
{
   StackObject<FSAStat> stat;
   FSAFileHandle fileHandle;
   auto fsaHandle = getFsaHandle();

   auto status = FSAOpenFile(fsaHandle, filename, "r", &fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileOpen;
   }

   status = FSAStatFile(fsaHandle, fileHandle, stat);
   if (status < FSAStatus::OK) {
      FSACloseFile(fsaHandle, fileHandle);
      return UCError::FileStat;
   }

   auto fileData = allocFileData(stat->size);
   if (!fileData) {
      FSACloseFile(fsaHandle, fileHandle);
      return UCError::Alloc;
   }

   status = FSAReadFile(fsaHandle,
                        fileData,
                        stat->size,
                        1,
                        fileHandle,
                        FSAReadFlag::None);
   if (status < FSAStatus::OK) {
      freeFileData(fileData, stat->size);
      FSACloseFile(fsaHandle, fileHandle);
      return UCError::FileRead;
   }

   status = FSACloseFile(fsaHandle, fileHandle);
   if (status < FSAStatus::OK) {
      freeFileData(fileData, stat->size);
      return UCError::FileClose;
   }

   *outBytesRead = stat->size;
   *outBuffer = fileData;
   return UCError::OK;
}

static UCError
writeFile(std::string_view filename,
          phys_ptr<uint8_t> buffer,
          uint32_t size)
{
   StackObject<FSAStat> stat;
   FSAFileHandle fileHandle;
   auto fsaHandle = getFsaHandle();

   if (!buffer) {
      if (FSARemove(fsaHandle, filename) < FSAStatus::OK) {
         return UCError::FileRemove;
      }

      return UCError::OK;
   }

   auto status = FSAOpenFile(fsaHandle, filename, "w", &fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileOpen;
   }

   status = FSAWriteFile(fsaHandle,
                         buffer,
                         size,
                         1,
                         fileHandle,
                         FSAWriteFlag::None);
   if (status < FSAStatus::OK) {
      FSACloseFile(fsaHandle, fileHandle);
      return UCError::FileWrite;
   }

   status = FSACloseFile(fsaHandle, fileHandle);
   if (status < FSAStatus::OK) {
      return UCError::FileClose;
   }

   return UCError::OK;
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

UCFileSys
getFileSys(std::string_view name)
{
   auto index = name.find_first_of(':');
   if (index == std::string_view::npos) {
      return UCFileSys::Sys;
   }

   auto prefix = name.substr(0, index);
   if (prefix.compare("sys") == 0) {
      return UCFileSys::Sys;
   } else if (prefix.compare("slc") == 0) {
      return UCFileSys::Slc;
   } else if (prefix.compare("ram") == 0) {
      return UCFileSys::Ram;
   }

   return UCFileSys::Invalid;
}

static UCFileSys
getFileSys(phys_ptr<UCItem> item)
{
   return getFileSys(std::string_view { phys_addrof(item->name).getRawPointer() });
}

std::string_view
getRootKey(std::string_view name)
{
   auto rootKeyStart = name.find_first_of(':');
   if (rootKeyStart == std::string_view::npos) {
      rootKeyStart = 0;
   } else {
      rootKeyStart += 1;
   }

   auto rootKeyEnd = name.find_first_of('.');
   if (rootKeyEnd == std::string_view::npos) {
      return name;
   }

   return name.substr(rootKeyStart, rootKeyEnd - rootKeyStart);
}

static std::string_view
getRootKey(phys_ptr<UCItem> item)
{
   return getRootKey(std::string_view { phys_addrof(item->name).getRawPointer() });
}

static UCError
getConfigPath(phys_ptr<UCItem> item,
              std::string_view fileSysPath,
              std::string &path)
{
   auto fileSys = getFileSys(item);
   if (fileSys == UCFileSys::Invalid) {
      return UCError::InvalidLocation;
   }

   auto rootKey = getRootKey(item);
   if (fileSysPath.length() + rootKey.length() + 7 > 64) {
      return UCError::StringTooLong;
   }

   path = fileSysPath;
   path += rootKey;
   path += ".xml";
   return UCError::OK;
}

static UCError
getItemPathName(phys_ptr<UCItem> item,
                std::string &outPath,
                std::string &outName)
{
   auto key = std::string_view { phys_addrof(item->name).getRawPointer() };
   auto colonPos = key.find_first_of(':');
   if (colonPos != std::string_view::npos) {
      key.remove_prefix(colonPos + 1);
   }

   auto lastDot = key.find_last_of('.');
   if (lastDot == std::string_view::npos) {
      outPath = {};
      outName = key;
   } else {
      outPath = key.substr(0, lastDot);
      outName = key.substr(lastDot + 1);
   }

   return UCError::OK;
}

/**
 * Ensure all items are in the same XML file and root key.
 */
static UCError
checkItems(phys_ptr<UCItem> items,
           uint32_t count)

{
   auto fileSys = getFileSys(items);
   if (fileSys == UCFileSys::Invalid) {
      return UCError::InvalidLocation;
   }

   auto rootKey = getRootKey(items);

   for (auto i = 0u; i < count; ++i) {
      auto item = phys_addrof(items[i]);

      if (getFileSys(item) != fileSys ||
          rootKey.compare(getRootKey(item)) != 0) {
         return UCError::RootKeysDiffer;
      }
   }

   return UCError::OK;
}

/**
 * Read the given items from file.
 */
UCError
readItems(std::string_view fileSysPath,
          phys_ptr<UCItem> items,
          uint32_t count,
          phys_ptr<IoctlVec> vecs)
{
   auto error = checkItems(items, count);
   if (error < UCError::OK) {
      return error;
   }

   auto path = std::string { };
   error = getConfigPath(items, fileSysPath, path);
   if (error < UCError::OK) {
      return error;
   }

   uint32_t fileSize;
   phys_ptr<uint8_t> fileBuffer;
   error = readFile(path, &fileSize, &fileBuffer);
   if (error < UCError::OK) {
      return error;
   }

   // Parse XML
   pugi::xml_document doc;
   auto parseResult = doc.load_buffer(fileBuffer.getRawPointer(), fileSize);
   freeFileData(fileBuffer, fileSize);

   if (!parseResult) {
      return UCError::MalformedXML;
   }

   for (auto i = 0u; i < count; ++i) {
      auto item = phys_addrof(items[i]);
      auto path = std::string { };
      auto name = std::string { };

      error = getItemPathName(item, path, name);
      if (error < UCError::OK) {
         item->error = error;
         continue;
      }

      // Convert to a pugixml element path
      auto elementPath = path;
      replace_all(elementPath, '.', '/');
      if (!path.empty()) {
         elementPath += '/';
      }
      elementPath += name;

      // Find item in the xml document
      auto node = doc.first_element_by_path(elementPath.c_str());
      if (!node) {
         item->error = UCError::KeyNotFound;
         continue;
      }

      // Verify the data type
      auto nodeType = getDataTypeByName(node.attribute("type").as_string());
      if (nodeType != item->dataType) {
         item->error = UCError::InvalidType;
         continue;
      }

      // Verify data length
      auto nodeLength = node.attribute("length").as_uint();
      if (item->dataType != UCDataType::Complex && !item->data) {
         item->error = UCError::InvalidParam;
         continue;
      }

      // Read access (TODO: Verify access??)
      auto nodeAccess = node.attribute("access");
      if (nodeAccess) {
         item->access = getAccessFromString(nodeAccess.value());
      }

      // Read data
      phys_ptr<void> itemData;

      if (vecs) {
         itemData = phys_cast<void *>(vecs[i + 1].paddr);
      } else {
         itemData = item->data;
      }

      switch (item->dataType) {
      case UCDataType::UnsignedByte:
         if (item->dataSize >= 1) {
            *phys_cast<uint8_t *>(itemData) = static_cast<uint8_t>(node.text().as_uint());
         } else {
            item->error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::UnsignedShort:
         if (item->dataSize >= 2) {
            *phys_cast<uint16_t *>(itemData) = static_cast<uint16_t>(node.text().as_uint());
         } else {
            item->error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::UnsignedInt:
         if (item->dataSize >= 4) {
            *phys_cast<uint32_t *>(itemData) = static_cast<uint32_t>(node.text().as_uint());
         } else {
            item->error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::SignedInt:
         if (item->dataSize >= 4) {
            *phys_cast<int32_t *>(itemData) = static_cast<int32_t>(node.text().as_int());
         } else {
            item->error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::Float:
         if (item->dataSize >= 4) {
            *phys_cast<float *>(itemData) = node.text().as_float();
         } else {
            item->error = UCError::InvalidParam;
            continue;
         }
         break;
      case UCDataType::String:
      {
         auto str = node.text().get();
         auto size = strlen(str);

         if (size < item->dataSize) {
            std::memcpy(item->data.getRawPointer(), str, size + 1);
            item->dataSize = static_cast<uint32_t>(size + 1);
         } else {
            item->error = UCError::StringTooLong;
            continue;
         }
         break;
      }
      case UCDataType::HexBinary:
      {
         auto src = node.text().get();
         auto size = strlen(src) / 2;

         if (size <= item->dataSize) {
            auto dst = phys_cast<uint8_t *>(itemData);

            for (auto i = 0u; i < size; ++i) {
               auto value = uint8_t { 0 };
               value |= src[i * 2 + 0] << 4;
               value |= src[i * 2 + 1];
               dst[i] = value;
            }

            item->dataSize = static_cast<uint32_t>(size);
         } else {
            item->error = UCError::StringTooLong;
            continue;
         }
         break;
      }
      case UCDataType::Complex:
         break;
      default:
         item->error = UCError::InvalidType;
         continue;
      }

      item->error = UCError::OK;
   }

   return UCError::OK;
}

/**
 * Write the given items to file.
 */
UCError
writeItems(std::string_view fileSysPath,
           phys_ptr<UCItem> items,
           uint32_t count,
           phys_ptr<IoctlVec> vecs)
{
   pugi::xml_document doc;
   uint32_t fileSize;
   phys_ptr<uint8_t> fileBuffer;

   auto error = checkItems(items, count);
   if (error < UCError::OK) {
      return error;
   }

   auto path = std::string { };
   error = getConfigPath(items, fileSysPath, path);
   if (error < UCError::OK) {
      return error;
   }

   // Try read existing file first
   error = readFile(path, &fileSize, &fileBuffer);
   if (error >= UCError::OK) {
      auto parseResult = doc.load_buffer(fileBuffer.getRawPointer(), fileSize);
      freeFileData(fileBuffer, fileSize);

      if (!parseResult) {
         return UCError::MalformedXML;
      }
   }

   // Apply modifications
   for (auto i = 0u; i < count; ++i) {
      auto item = phys_addrof(items[i]);
      auto keyPath = std::string { };
      auto keyName = std::string { };

      error = getItemPathName(item, keyPath, keyName);
      if (error < UCError::OK) {
         item->error = error;
         continue;
      }

      if (keyPath.empty()) {
         auto node = doc.first_child();
         if (node) {
            if (keyName != node.name()) {
               item->error = UCError::RootKeysDiffer;
               continue;
            }

            if (strcmp(node.attribute("type").as_string(), "complex")) {
               item->error = UCError::RootKeysDiffer;
               continue;
            }
         } else {
            node = doc.append_child(keyName.c_str());
            node.append_attribute("type").set_value("complex");

            if (item->access) {
               node.append_attribute("access");
            }
         }

         if (item->access) {
            node.attribute("access").set_value(getAccessString(item->access).c_str());
         }
      } else {
         auto nodePath = keyPath;
         replace_all(nodePath, '.', '/');

         auto parent = doc.first_element_by_path(nodePath.c_str());
         if (!parent) {
            item->error = UCError::KeyNotFound;
            continue;
         }

         auto parentType = parent.attribute("type");
         if (strcmp(parentType.as_string(), "complex")) {
            item->error = UCError::KeyNotFound;
            continue;
         }

         auto node = parent.child(keyName.c_str());
         if (!node) {
            node = parent.append_child(keyName.c_str());
            node.append_attribute("type");

            if (item->dataType != UCDataType::Complex) {
               node.append_attribute("length");
            }

            if (item->access) {
               node.append_attribute("access");
            }
         }

         auto dataTypeName = getDataTypeName(item->dataType);
         if (!dataTypeName) {
            item->error = UCError::InvalidType;
            continue;
         }

         node.attribute("type").set_value(dataTypeName);

         if (item->dataType != UCDataType::Complex) {
            node.attribute("length").set_value(item->dataSize);
         }

         if (item->access) {
            node.attribute("access").set_value(getAccessString(item->access).c_str());
         }

         phys_ptr<void> itemData;

         if (vecs) {
            itemData = phys_cast<void *>(vecs[i + 1].paddr);
         } else {
            itemData = item->data;
         }

         switch (item->dataType) {
         case UCDataType::UnsignedByte:
            if (item->data) {
               node.text().set(*phys_cast<uint8_t *>(itemData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::UnsignedShort:
            if (item->data) {
               node.text().set(*phys_cast<uint16_t *>(itemData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::UnsignedInt:
            if (item->data) {
               node.text().set(*phys_cast<uint32_t *>(itemData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::SignedInt:
            if (item->data) {
               node.text().set(*phys_cast<int32_t *>(itemData));
            } else {
               node.text().set(0);
            }
            break;
         case UCDataType::Float:
            if (item->data) {
               node.text().set(*phys_cast<float *>(itemData));
            } else {
               node.text().set(0.0f);
            }
            break;
         case UCDataType::String:
            if (item->data) {
               // TODO: Check text format, maybe utf8/utf16 etc?
               node.text().set(phys_cast<char *>(itemData).getRawPointer());
            } else {
               node.text().set("");
            }
            break;
         case UCDataType::HexBinary:
            if (item->data) {
               node.text().set(to_string(phys_cast<uint8_t *>(itemData).getRawPointer(), item->dataSize).c_str());
            } else {
               std::string value(static_cast<size_t>(item->dataSize * 2), '0');
               node.text().set(value.c_str());
            }
            break;
         default:
            item->error = UCError::InvalidType;
            continue;
         }
      }

      item->error = UCError::OK;
   }

   // Write modified config to file
   std::stringstream ss;
   doc.save(ss, "  ", 1, pugi::encoding_utf8);

   auto xmlStr = ss.str();

   fileSize = static_cast<uint32_t>(xmlStr.size() + 1);
   fileBuffer = allocFileData(fileSize);
   if (!fileBuffer) {
      return UCError::Alloc;
   }

   error = writeFile(path, fileBuffer, fileSize);
   freeFileData(fileBuffer, fileSize);
   return error;
}

/**
 * List up to count items from xml
 *
 * Sets name, access, dataSize, dataType
 */
UCError
listItems(phys_ptr<UCItem> items,
          uint32_t count)
{
   return UCError::Unsupported;
}

/**
 * Get access, error, dataSize, dataType for specified items
 */
UCError
queryItems(phys_ptr<UCItem> items,
           uint32_t count)
{
   return UCError::Unsupported;
}

/**
 * Delete specific items from the config xml
 */
UCError
deleteItems(std::string_view fileSysPath,
            phys_ptr<UCItem> items,
            uint32_t count)
{
   auto error = checkItems(items, count);
   if (error < UCError::OK) {
      return error;
   }

   auto path = std::string { };
   error = getConfigPath(items, fileSysPath, path);
   if (error < UCError::OK) {
      return error;
   }

   uint32_t fileSize;
   phys_ptr<uint8_t> fileBuffer;
   error = readFile(path, &fileSize, &fileBuffer);
   if (error < UCError::OK) {
      return error;
   }

   // Parse XML
   pugi::xml_document doc;
   auto parseResult = doc.load_buffer(fileBuffer.getRawPointer(), fileSize);
   freeFileData(fileBuffer, fileSize);

   if (!parseResult) {
      return UCError::MalformedXML;
   }

   for (auto i = 0u; i < count; ++i) {
      auto item = phys_addrof(items[i]);
      auto path = std::string { };
      auto name = std::string { };

      error = getItemPathName(item, path, name);
      if (error < UCError::OK) {
         item->error = error;
         continue;
      }

      // Convert to a pugixml element path
      auto elementPath = path;
      replace_all(elementPath, '.', '/');
      if (!path.empty()) {
         elementPath += '/';
      }
      elementPath += name;

      // Find item in the xml document
      auto node = doc.first_element_by_path(elementPath.c_str());
      if (!node) {
         item->error = UCError::KeyNotFound;
         continue;
      }

      // Verify the data type
      auto nodeType = getDataTypeByName(node.attribute("type").as_string());
      if (nodeType != item->dataType) {
         item->error = UCError::InvalidType;
         continue;
      }

      // Verify data length
      auto nodeLength = node.attribute("length").as_uint();
      if (item->dataType != UCDataType::Complex && !item->data) {
         item->error = UCError::InvalidParam;
         continue;
      }

      // Remove node!
      node.parent().remove_child(node.name());
      item->error = UCError::OK;
   }

   // Write modified config to file
   std::stringstream ss;
   doc.save(ss, "  ", 1, pugi::encoding_utf8);

   auto xmlStr = ss.str();

   fileSize = static_cast<uint32_t>(xmlStr.size() + 1);
   fileBuffer = allocFileData(fileSize);
   if (!fileBuffer) {
      return UCError::Alloc;
   }

   error = writeFile(path, fileBuffer, fileSize);
   freeFileData(fileBuffer, fileSize);
   return error;
}

/**
 * Delete the whole config!
 */
UCError
deleteRoot(phys_ptr<UCItem> items,
           uint32_t count)
{
   return UCError::Unsupported;
}

} // namespace ios::auxil
