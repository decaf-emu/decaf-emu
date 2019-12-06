#include "ios_nsec_nssl_certstore.h"
#include "ios_nsec_log.h"

#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "ios/ios_error.h"
#include "ios/ios_stackobject.h"

#include <array>
#include <charconv>
#include <common/strutils.h>
#include <string_view>

using namespace ios::fs;
using namespace ios::kernel;

namespace ios::nsec::internal
{

struct StaticCertStoreData
{
   be2_val<FSAHandle> fsaHandle;
   be2_array<CertStoreMetaData, MaxNumCertificates> metaData;
};

static phys_ptr<StaticCertStoreData> sCertStoreData = nullptr;

bool
checkCertPermission(phys_ptr<CertStoreMetaData> certMetaData,
                    TitleId titleId,
                    ProcessId processId,
                    uint64_t caps)
{
   if (caps & certMetaData->capabilityMask) {
      return true;
   }

   for (auto tid : certMetaData->titleIds) {
      if (tid == -1) {
         break;
      }

      if (tid == titleId) {
         return true;
      }
   }

   for (auto pid : certMetaData->processIds) {
      if (pid == static_cast<ProcessId>(-1)) {
         break;
      } else if (pid == static_cast<ProcessId>(-4)) {
         return true;
      } else if (pid == static_cast<ProcessId>(-2) && processId < ProcessId::TEST) {
         return true;
      } else if (pid == static_cast<ProcessId>(-3) && processId >= ProcessId::COSKERNEL) {
         return true;
      } else if (processId == ProcessId::TEST) {
         return true;
      } else if (processId == pid) {
         return true;
      }
   }

   return false;
}

bool
checkCertExportable(phys_ptr<CertStoreMetaData> certMetaData)
{
   return !!(certMetaData->properties & NSSLCertProperties::Exportable);
}

std::optional<uint32_t>
getCertFileSize(phys_ptr<CertStoreMetaData> certMetaData,
                int32_t pathIndex)
{
   if (pathIndex >= certMetaData->numPaths) {
      return {};
   }

   if (pathIndex > 0 && !(certMetaData->properties & NSSLCertProperties::HasSecondPath)) {
      return {};
   }

   auto path = std::string { "/vol/storage_mlc01/sys/title/0005001b/10054000/content/" };
   if (pathIndex == 0) {
      path += phys_addrof(certMetaData->path1).get();
   } else if (pathIndex == 1) {
      path += phys_addrof(certMetaData->path2).get();
   }

   auto stat = StackObject<FSAStat> { };
   auto error = FSAGetStat(sCertStoreData->fsaHandle, path, stat);
   if (error < FSAStatus::OK) {
      return {};
   }

   return static_cast<uint32_t>(stat->size);
}

std::optional<uint32_t>
getCertFileData(phys_ptr<CertStoreMetaData> certMetaData,
                int32_t pathIndex,
                phys_ptr<uint8_t> certBuffer,
                uint32_t certBufferSize)
{
   // Read file data 4096 bytes at a time into allocated buffer copying into certbuiffer
   auto heapBufferSize = std::min(4096u, certBufferSize);
   auto heapBuffer = IOS_HeapAllocAligned(CrossProcessHeapId, heapBufferSize, 0x40u);
   if (!heapBuffer) {
      return {};
   }

   auto path = std::string { "/vol/storage_mlc01/sys/title/0005001b/10054000/content/" };
   if (pathIndex == 0) {
      path += phys_addrof(certMetaData->path1).get();
   } else if (pathIndex == 1) {
      path += phys_addrof(certMetaData->path2).get();
   }

   auto fileHandle = FSAFileHandle { -1 };
   auto error = FSAOpenFile(sCertStoreData->fsaHandle, path, "r", &fileHandle);
   if (error < FSAStatus::OK) {
      return {};
   }

   auto certSize = 0;

   while (true) {
      error = FSAReadFile(sCertStoreData->fsaHandle, heapBuffer, 1, heapBufferSize, fileHandle, FSAReadFlag::None);
      if (error < 0) {
         break;
      }

      auto bytesRead = static_cast<uint32_t>(error);
      std::memcpy(certBuffer.get() + certSize, heapBuffer.get(), error);
      certSize += bytesRead;

      if (bytesRead < heapBufferSize) {
         // Reached EOF
         break;
      }
   }

   IOS_HeapFree(CrossProcessHeapId, heapBuffer);

   if (error < FSAStatus::OK && error != FSAStatus::EndOfFile) {
      return {};
   }

   return certSize;
}

phys_ptr<CertStoreMetaData>
lookupCertMetaData(NSSLCertID id)
{
   for (auto &cert : sCertStoreData->metaData) {
      if (cert.id == id) {
         return phys_addrof(cert);
      } else if (cert.id == -1) {
         break;
      }
   }

   return nullptr;
}

Error
loadCertstoreMetadata()
{
   auto openError = FSAOpen();
   if (openError < FSAStatus::OK) {
      nsecLog->warn("loadCertstoreMetadata: FSAOpen failed with error {}", openError);
      return openError;
   }
   sCertStoreData->fsaHandle = static_cast<FSAHandle>(openError);

   // TODO: Use MCP_SearchTitle to find title path
   auto path = "/vol/storage_mlc01/sys/title/0005001b/10054000/content/certstore_metadata.txt";

   auto stat = StackObject<FSAStat> { };
   auto error = FSAGetStat(sCertStoreData->fsaHandle, path, stat);
   if (error < FSAStatus::OK) {
      nsecLog->warn("loadCertstoreMetadata: FSAStat failed with error {} on {}", error, path);
      return static_cast<Error>(error);
   }

   auto size = stat->size ? stat->size.value() : 10240u;
   auto fileBuffer = IOS_HeapAlloc(LocalProcessHeapId, size);
   if (!fileBuffer) {
      nsecLog->warn("loadCertstoreMetadata: Failed to allocate file buffer of size {}", size);
      return Error::QFull;
   }

   auto fileHandle = FSAFileHandle { -1 };
   error = FSAOpenFile(sCertStoreData->fsaHandle, path, "r", &fileHandle);
   if (error < FSAStatus::OK) {
      nsecLog->warn("loadCertstoreMetadata: FSAOpenFile failed with error {} on {}", error, path);
      return static_cast<Error>(error);
   }

   error = FSAReadFile(sCertStoreData->fsaHandle, fileBuffer, 1, size, fileHandle, FSAReadFlag::None);
   if (error < FSAStatus::OK) {
      nsecLog->warn("loadCertstoreMetadata: FSAReadFile failed with error {}", error);
      FSACloseFile(sCertStoreData->fsaHandle, fileHandle);
      return static_cast<Error>(error);
   }

   size = static_cast<uint32_t>(error);
   FSACloseFile(sCertStoreData->fsaHandle, fileHandle);

   // Now parse fileBuffer, size bytes for metadata
   auto contents = std::string_view { phys_cast<char *>(fileBuffer).get(), size };
   auto position = size_t { 0 };
   auto certIndex = 0u;
   auto lineIndex = 1u;

   // Iteratre through lines of fileBuffer
   while (position < size) {
      auto lineEndPosition = contents.find_first_of('\n', position);
      if (lineEndPosition == std::string_view::npos) {
         lineEndPosition = size;
      }

      auto line = trim(contents.substr(position, lineEndPosition - position));
      if (!line.empty() && line[0] != '#') {
         auto linePosition = size_t { 0 };
         struct {
            bool hasId = false;
            bool hasType = false;
            bool hasEncoding = false;
            bool hasProperties = false;
            bool hasCapabilityMask = false;
            bool hasProcessIds = false;
            bool hasTitleIds = false;
            bool hasPaths = false;
            bool hasGroups = false;
            bool hasRawE0Size = false;
            bool hasRawE1Size = false;
            bool hasChecksum = false;

            long id = 0;
            long type = 0;
            long encoding = 0;
            long properties = 0;
            unsigned long long capabilityMask = 0;
            std::array<long, 33> processIds;
            std::array<long long, 33> titleIds;
            long numPaths = 0;
            std::string_view path1;
            std::string_view path2;
            long groups = 0;
            long rawE0Size = 0;
            long rawE1Size = 0;
         } certData;
         certData.titleIds[0] = -1;
         certData.processIds[0] = -1;

         // Iterator through the line splitting by ;
         while (linePosition < line.size()) {
            auto kvEndPosition = std::min(line.find_first_of(';', linePosition), line.size());
            auto kvPair = line.substr(linePosition, kvEndPosition - linePosition);

            // Split the key=value pair
            auto eqPos = kvPair.find_first_of('=');
            if (eqPos != std::string_view::npos && eqPos + 1 < kvPair.size()) {
               auto key = trim(kvPair.substr(0, eqPos));
               auto value = trim(kvPair.substr(eqPos + 1));

               if (iequals(key, "ID")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.id, 10);
                  certData.hasId = true;
               } else if (iequals(key, "TYPE")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.type, 10);
                  certData.hasType = true;
               } else if (iequals(key, "ENCODING")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.encoding, 10);
                  certData.hasEncoding = true;
               } else if (iequals(key, "PROPERTIES")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.properties, 16);
                  certData.hasProperties = true;
               } else if (iequals(key, "CAPABILITY_MASK")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.capabilityMask, 16);
                  certData.hasCapabilityMask = true;
               } else if (iequals(key, "PID")) {
                  auto pidPosition = size_t { 0 };
                  auto pidIndex = 0u;
                  while (pidPosition < value.size() && pidIndex < certData.processIds.size() - 1) {
                     auto pidEndPosition = std::min(value.find_first_of(','), value.size());
                     auto pid = value.substr(pidPosition, pidEndPosition - pidPosition);
                     std::from_chars(pid.data(), pid.data() + pid.size(), certData.processIds[pidIndex++], 10);
                     pidPosition = pidEndPosition + 1;
                  }

                  certData.processIds[pidIndex] = -1;
                  certData.hasProcessIds = true;
               } else if (iequals(key, "TID")) {
                  auto tidPosition = size_t {  0 };
                  auto tidIndex = 0u;
                  while (tidPosition < value.size() && tidIndex < certData.titleIds.size() - 1) {
                     auto tidEndPosition = std::min(value.find_first_of(','), value.size());
                     auto tid = value.substr(tidPosition, tidEndPosition - tidPosition);
                     std::from_chars(tid.data(), tid.data() + tid.size(), certData.titleIds[tidIndex++], 16);
                     tidPosition = tidEndPosition + 1;
                  }

                  certData.titleIds[tidIndex] = -1;
                  certData.hasTitleIds = true;
               } else if (iequals(key, "PATHS")) {
                  auto path2Position = std::min(value.find_first_of(','), value.size());
                  certData.path1 = value.substr(0, path2Position);
                  if (path2Position + 1 < value.size()) {
                     certData.path2 = value.substr(path2Position + 1);
                  }

                  if (!certData.path1.empty()) {
                     certData.numPaths += 1;
                  }

                  if (!certData.path2.empty()) {
                     certData.numPaths += 1;
                  }

                  certData.hasPaths = true;
               } else if (iequals(key, "GROUPS")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.groups, 10);
                  certData.hasGroups = true;
               } else if (iequals(key, "RAW_E0_SIZE") || iequals(key, "RAW_SIZE")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.rawE0Size, 10);
                  certData.hasRawE0Size = true;
               } else if (iequals(key, "RAW_E1_SIZE") || iequals(key, "KEYSIZEDER")) {
                  std::from_chars(value.data(), value.data() + value.size(), certData.rawE1Size, 10);
                  certData.hasRawE1Size = true;
               } else {
                  nsecLog->warn("CERTSTORE: Unrecognized property name '{}' on line {}", key, lineIndex);
               }
            }

            linePosition = kvEndPosition + 1;
         }

         if (!certData.hasId) {
            nsecLog->warn("CERTSTORE: ID not specified on line {}", lineIndex);
         } else if (!certData.hasType) {
            nsecLog->warn("CERTSTORE: TYPE not specified on line {}", lineIndex);
         } else if (!certData.hasEncoding) {
            nsecLog->warn("CERTSTORE: ENCODING not specified on line {}", lineIndex);
         } else if (!certData.hasProperties) {
            nsecLog->warn("CERTSTORE: PROPERTIES not specified on line {}", lineIndex);
         } else if (!certData.hasCapabilityMask) {
            nsecLog->warn("CERTSTORE: CAPABILITY_MASK not specified on line {}", lineIndex);
         } else if (!certData.hasProcessIds && !certData.hasTitleIds) {
            nsecLog->warn("CERTSTORE: Neither PID nor TID specified on line {}", lineIndex);
         } else if (!certData.hasPaths) {
            nsecLog->warn("CERTSTORE: PATHS not specified on line {}", lineIndex);
         } else if (!certData.hasGroups) {
            nsecLog->warn("CERTSTORE: GROUPS not specified on line {}", lineIndex);
         } else if (!certData.hasRawE0Size && (certData.properties & 4)) {
            nsecLog->warn("CERTSTORE: RAW_E0_SIZE not specified on line {} for encrypted entity", lineIndex);
         } else if (!certData.hasRawE1Size && (certData.properties & 1)) {
            nsecLog->warn("CERTSTORE: KEYSIZEDER/RAW_E1_SIZE not specified on line {}", lineIndex);
         } else {
            auto &cert = sCertStoreData->metaData[certIndex++];
            cert.id = static_cast<int32_t>(certData.id);
            cert.type = static_cast<int32_t>(certData.type);
            cert.encoding = static_cast<NSSLCertEncoding>(certData.encoding);
            cert.properties = static_cast<NSSLCertProperties>(certData.properties);
            cert.groups = static_cast<int32_t>(certData.groups);
            cert.capabilityMask = static_cast<uint64_t>(certData.capabilityMask);
            cert.numPaths = static_cast<int32_t>(certData.numPaths);
            cert.path1 = certData.path1;
            cert.path2 = certData.path2;
            cert.rawE0Size = static_cast<int32_t>(certData.rawE0Size);
            cert.rawE1Size = static_cast<int32_t>(certData.rawE1Size);

            for (auto i = 0u; i < certData.processIds.size(); ++i) {
               cert.processIds[i] = static_cast<ProcessId>(certData.processIds[i]);
            }

            for (auto i = 0u; i < certData.titleIds.size(); ++i) {
               cert.titleIds[i] = static_cast<TitleId>(certData.titleIds[i]);
            }
         }
      }

      if (certIndex == sCertStoreData->metaData.size()) {
         nsecLog->warn("CERTSTORE: Maximum Entity Limit ({}) Reached !",
                       sCertStoreData->metaData.size());
         break;
      }

      position = lineEndPosition + 1;
      ++lineIndex;
   }

   if (certIndex < sCertStoreData->metaData.size()) {
      sCertStoreData->metaData[certIndex].id = -1;
   }

   IOS_HeapFree(LocalProcessHeapId, fileBuffer);
   return Error::OK;
}

void
initialiseStaticCertStoreData()
{
   sCertStoreData = allocProcessStatic<StaticCertStoreData>();
}

} // namespace ios::nsec::internal
