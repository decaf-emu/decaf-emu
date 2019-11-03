#include "ios_fpd_act_accountmanager.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_log.h"

#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "nn/ffl/nn_ffl_miidata.h"
#include "nn/act/nn_act_result.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <string_view>

using namespace nn::act;
using namespace nn::ffl;
using namespace ios::fs;
using namespace ios::kernel;

namespace ios::fpd::internal
{

template<typename ReadKeyValueCallback>
static FSAStatus
readPropertyFile(std::string_view path,
                 std::string_view typeName,
                 ReadKeyValueCallback readKeyValueCallback)
{
   auto fsaHandle = getActFsaHandle();
   auto fileHandle = FSAFileHandle { -1 };
   auto result = FSAOpenFile(fsaHandle, path, "r", &fileHandle);
   if (result < 0) {
      internal::fpdLog->debug("Could not open {}", path);
      return result;
   }

   auto fileBufferSize = 1024u;
   auto fileBuffer = phys_cast<char *>(IOS_HeapAllocAligned(CrossProcessHeapId, fileBufferSize, 0x40));
   if (!fileBuffer) {
      internal::fpdLog->debug("Could not allocate file buffer");
      FSACloseFile(fsaHandle, fileHandle);
      return FSAStatus::OutOfResources;
   }

   auto bufferPos = 0u;
   auto lineStartPos = 0u;
   auto readBufferOffset = 0u;
   auto readFileHeader = false;

   while (true) {
      result = FSAReadFile(fsaHandle, fileBuffer + readBufferOffset,
                           1, fileBufferSize - readBufferOffset,
                           fileHandle, FSAReadFlag::None);
      if (result < 0) {
         internal::fpdLog->debug("FSAReadFile failed");
         IOS_HeapFree(CrossProcessHeapId, fileBuffer);
         FSACloseFile(fsaHandle, fileHandle);
         return result;
      }

      auto bytesRead = static_cast<uint32_t>(result);
      auto bufferPos = readBufferOffset;

      auto eof = bytesRead < (fileBufferSize - readBufferOffset);
      if (bytesRead == 0) {
         // Hit exactly eof with the previous read
         break;
      }

      while (bufferPos < bytesRead) {
         if (fileBuffer[bufferPos] == '\n') {
            auto line = std::string_view {
               fileBuffer.get() + lineStartPos, bufferPos - lineStartPos
            };

            if (!readFileHeader) {
               // Check that line starts with typeName
               if (line.substr(0, typeName.size()).compare(typeName) != 0) {
                  result = FSAStatus::DataCorrupted;
                  eof = true;
                  break;
               }

               readFileHeader = true;
            } else {
               // Should be a Key=Value line
               auto separator = line.find_first_of('=');
               if (separator == std::string_view::npos) {
                  result = FSAStatus::DataCorrupted;
                  eof = true;
                  break;
               }

               auto key = line.substr(0, separator);
               auto value = line.substr(separator + 1);
               if (key.empty()) {
                  result = FSAStatus::DataCorrupted;
                  eof = true;
                  break;
               }

               readKeyValueCallback(key, value);
            }

            lineStartPos = bufferPos + 1;
         }

         ++bufferPos;
      }

      if (eof) {
         // Reached end of file with last read
         break;
      }

      if (lineStartPos == 0u) {
         // Could not fit whole line in our buffer, must be a bad file
         result = FSAStatus::DataCorrupted;
         break;
      }

      if (bufferPos == bytesRead) {
         std::memmove(fileBuffer.get(),
                      fileBuffer.get() + lineStartPos,
                      sizeof(bytesRead - lineStartPos));
         readBufferOffset = bytesRead - lineStartPos;
         lineStartPos = 0u;
      }
   }

   if (result > 0) {
      result = FSAStatus::OK;
   }

   FSACloseFile(fsaHandle, fileHandle);
   IOS_HeapFree(CrossProcessHeapId, fileBuffer);
   return result;
}

void
parseBoolProperty(std::string_view value, be2_val<uint8_t> &out)
{
   int temporary = 0;
   std::from_chars(value.data(), value.data() + value.size(), temporary, 16);
   out = temporary != 0;
}

template<typename Type>
void
parseIntegerProperty(std::string_view value, be2_val<Type> &out)
{
   Type result { 0 };
   std::from_chars(value.data(), value.data() + value.size(), result, 16);
   out = result;
}

void
parseHexString(std::string_view value, phys_ptr<uint8_t> out, size_t size)
{
   std::memset(out.get(), 0, sizeof(size));

   for (auto i = 0; i < value.size() && i / 2 < size; ++i) {
      auto nibble = uint8_t { 0 };
      if (value[i] >= 'a' && value[i] <= 'f') {
         nibble = (value[i] - 'a') + 0xa;
      } else if (value[i] >= 'A' && value[i] <= 'F') {
         nibble = (value[i] - 'A') + 0xA;
      } else if (value[i] >= '0' && value[i] <= '9') {
         nibble = (value[i] - '0') + 0x0;
      }

      out[i / 2] |= static_cast<uint8_t>((i % 2) ? nibble : (nibble << 4));
   }
}

template<uint32_t Size>
void
parseStringProperty(std::string_view value, be2_array<char, Size> &out)
{
   out.fill(0);
   std::memcpy(phys_addrof(out).get(), value.data(),
               std::min<std::size_t>(value.size(), out.size()));

   if (value.size() == out.size()) {
      out.back() = '\0';
   }
}

bool
loadTransferableIdManager(phys_ptr<TransferableIdManager> transferableIdManager)
{
   auto result = readPropertyFile(
      "/vol/storage_mlc01/usr/save/system/act/transid.dat", "TransferableIdManager",
      [transferableIdManager=transferableIdManager.get()]
      (std::string_view key, std::string_view value) {
         if (key == "Counter") {
            parseIntegerProperty(value, transferableIdManager->counter);
         }
      });

   if (result == FSAStatus::OK) {
      return true;
   }

   transferableIdManager->counter = 0u;
   return false;
}

bool
loadUuidManager(phys_ptr<UuidManager> uuidManager)
{
   auto result = readPropertyFile(
      "/vol/storage_mlc01/usr/save/system/act/uuid.dat", "UuidManager",
      [uuidManager=uuidManager.get()]
      (std::string_view key, std::string_view value) {
         if (key == "ClockSequence") {
            parseIntegerProperty(value, uuidManager->clockSequence);
         } else if (key == "LastTime") {
            parseIntegerProperty(value, uuidManager->lastTime);
         }
      });

   if (result == FSAStatus::OK) {
      return true;
   }

   uuidManager->lastTime = 0ll;
   uuidManager->clockSequence = 0;
   return false;
}

bool
loadPersistentIdManager(phys_ptr<PersistentIdManager> persistentIdManager)
{
   auto result = readPropertyFile(
      "/vol/storage_mlc01/usr/save/system/act/persisid.dat", "PersistentIdManager",
      [persistentIdManager=persistentIdManager.get()]
      (std::string_view key, std::string_view value) {
         if (key == "PersistentIdHead") {
            parseIntegerProperty(value, persistentIdManager->persistentIdHead);
         }
      });

   if (result == FSAStatus::OK) {
      return true;
   }

   persistentIdManager->persistentIdHead = 0x80000000;
   return false;
}


static void
parseNnasSubDomain(std::string_view subDomain,
                   be2_val<uint32_t> &outNnasType)
{
   if (subDomain.compare("game-dev.") == 0) {
      outNnasType = 1u;
   } else if (subDomain.compare("system-dev.") == 0) {
      outNnasType = 2u;
   } else if (subDomain.compare("library-dev.") == 0) {
      outNnasType = 3u;
   } else if (subDomain.compare("staging.") == 0) {
      outNnasType = 4u;
   } else {
      outNnasType = 0u;
   }
}

static void
parseNnasNfsEnv(std::string_view nfsEnv,
                be2_val<uint32_t> &outNfsType,
                be2_val<uint8_t> &outNfsNo)
{
   switch (nfsEnv[0]) {
   case 'D':
      outNfsType = 1u;
      break;
   case 'J':
      outNfsType = 4u;
      break;
   case 'L':
      outNfsType = 0u;
      break;
   case 'S':
      outNfsType = 2u;
      break;
   case 'T':
      outNfsType = 3u;
      break;
   default:
      outNfsNo = uint8_t { 1 };
      outNfsType = 0u;
      return;
   }

   outNfsNo = std::clamp<uint8_t>(nfsEnv[1] - '0', 1, 9);
}

bool
loadAccountManager(phys_ptr<AccountManager> accountManager)
{
   auto result = readPropertyFile(
      "/vol/storage_mlc01/usr/save/system/act/common.dat", "AccountManager",
      [accountManager](std::string_view key, std::string_view value) {
         if (key == "PersistentIdList") {
            auto start = size_t { 0 };
            auto end = value.find("\\0");
            auto index = 0;
            accountManager->persistentIdList.fill(0);

            while (end != std::string_view::npos) {
               auto persistentId = value.substr(start, end - start);
               parseIntegerProperty(persistentId, accountManager->persistentIdList[index++]);
               start = end + 2;
               end = value.find("\\0", start);
            }
         } else if (key == "DefaultAccountPersistentId") {
            parseIntegerProperty(value, accountManager->defaultAccountPersistentId);
         } else if (key == "CommonTransferableIdBase") {
            parseIntegerProperty(value, accountManager->commonTransferableIdBase);
         } else if (key == "CommonUuid") {
            parseHexString(value, phys_addrof(accountManager->commonUuid), accountManager->commonUuid.size());
         } else if (key == "IsApplicationUpdateRequired") {
            parseBoolProperty(value, accountManager->isApplicationUpdateRequired);
         } else if (key == "DefaultNnasType") {
            parseIntegerProperty(value, accountManager->defaultNnasType);
         } else if (key == "DefaultNfsType") {
            parseIntegerProperty(value, accountManager->defaultNfsType);
         } else if (key == "DefaultNfsNo") {
            parseIntegerProperty(value, accountManager->defaultNfsNo);
         } else if (key == "DefaultNnasSubDomain") {
            parseStringProperty(value, accountManager->defaultNnasSubDomain);
         } else if (key == "DefaultNnasNfsEnv") {
            parseStringProperty(value, accountManager->defaultNnasNfsEnv);
         }
      });

   if (result == FSAStatus::OK) {
      return true;
   }

   accountManager->persistentIdList.fill(0);
   accountManager->defaultAccountPersistentId = 0u;
   accountManager->commonTransferableIdBase = 0u;
   accountManager->commonUuid.fill(0);
   accountManager->isApplicationUpdateRequired = false;

   accountManager->defaultNnasSubDomain.fill(0);
   parseNnasSubDomain(phys_addrof(accountManager->defaultNnasSubDomain).get(),
                      accountManager->defaultNnasType);

   accountManager->defaultNnasSubDomain.fill(0);
   std::strncpy(phys_addrof(accountManager->defaultNnasSubDomain).get(), "L1",
                accountManager->defaultNnasSubDomain.size());
   parseNnasNfsEnv(phys_addrof(accountManager->defaultNnasNfsEnv).get(),
                   accountManager->defaultNfsType,
                   accountManager->defaultNfsNo);
   return false;
}

bool
loadAccountInstance(phys_ptr<AccountInstance> accountInstance,
                    PersistentId persistentId)
{
   auto path = fmt::format("/vol/storage_mlc01/usr/save/system/act/{:08x}/account.dat", persistentId);
   auto result = readPropertyFile(
      path, "AccountInstance",
      [accountInstance](std::string_view key, std::string_view value) {
         if (key == "PersistentId") {
            parseIntegerProperty(value, accountInstance->persistentId);
         } else if (key == "TransferableIdBase") {
            parseIntegerProperty(value, accountInstance->transferableIdBase);
         } else if (key == "Uuid") {
            parseHexString(value, phys_addrof(accountInstance->uuid), accountInstance->uuid.size());
         } else if (key == "ParentalControlSlotNo") {
            parseIntegerProperty(value, accountInstance->parentalControlSlotNo);
         } else if (key == "MiiData") {
            parseHexString(value, phys_cast<uint8_t *>(phys_addrof(accountInstance->miiData)), sizeof(accountInstance->miiData));
         } else if (key == "MiiName") {
            parseHexString(value, phys_cast<uint8_t *>(phys_addrof(accountInstance->miiName)), accountInstance->miiName.size() * 2);
         } else if (key == "IsMiiUpdated") {
            parseBoolProperty(value, accountInstance->isMiiUpdated);
         } else if (key == "AccountId") {
            parseStringProperty(value, accountInstance->accountId);
         } else if (key == "BirthYear") {
            parseIntegerProperty(value, accountInstance->birthYear);
         } else if (key == "BirthMonth") {
            parseIntegerProperty(value, accountInstance->birthMonth);
         } else if (key == "BirthDay") {
            parseIntegerProperty(value, accountInstance->birthDay);
         } else if (key == "Gender") {
            parseIntegerProperty(value, accountInstance->gender);
         } else if (key == "IsMailAddressValidated") {
            parseBoolProperty(value, accountInstance->isMailAddressValidated);
         } else if (key == "EmailAddress") {
            parseStringProperty(value, accountInstance->emailAddress);
         } else if (key == "Country") {
            parseIntegerProperty(value, accountInstance->country);
         } else if (key == "SimpleAddressId") {
            parseIntegerProperty(value, accountInstance->simpleAddressId);
         } else if (key == "TimeZoneId") {
            parseStringProperty(value, accountInstance->timeZoneId);
         } else if (key == "UtcOffset") {
            parseIntegerProperty(value, accountInstance->utcOffset);
         } else if (key == "PrincipalId") {
            parseIntegerProperty(value, accountInstance->principalId);
         } else if (key == "NfsPassword") {
            parseStringProperty(value, accountInstance->nfsPassword);
         } else if (key == "EciVirtualAccount") {
            parseStringProperty(value, accountInstance->eciVirtualAccount);
         } else if (key == "NeedsToDownloadMiiImage") {
            parseBoolProperty(value, accountInstance->needsToDownloadMiiImage);
         } else if (key == "MiiImageUrl") {
            parseStringProperty(value, accountInstance->miiImageUrl);
         } else if (key == "AccountPasswordHash") {
            parseHexString(value, phys_addrof(accountInstance->accountPasswordHash), accountInstance->accountPasswordHash.size());
         } else if (key == "IsPasswordCacheEnabled") {
            parseBoolProperty(value, accountInstance->isPasswordCacheEnabled);
         } else if (key == "AccountPasswordCache") {
            parseHexString(value, phys_addrof(accountInstance->accountPasswordCache), accountInstance->accountPasswordCache.size());
         } else if (key == "NnasType") {
            parseIntegerProperty(value, accountInstance->nnasType);
         } else if (key == "NfsType") {
            parseIntegerProperty(value, accountInstance->nfsType);
         } else if (key == "NfsNo") {
            parseIntegerProperty(value, accountInstance->nfsNo);
         } else if (key == "NnasSubDomain") {
            parseStringProperty(value, accountInstance->nnasSubDomain);
         } else if (key == "NnasNfsEnv") {
            parseStringProperty(value, accountInstance->nnasNfsEnv);
         } else if (key == "IsPersistentIdUploaded") {
            parseBoolProperty(value, accountInstance->isPersistentIdUploaded);
         } else if (key == "IsConsoleAccountInfoUploaded") {
            parseBoolProperty(value, accountInstance->isConsoleAccountInfoUploaded);
         } else if (key == "LastAuthenticationResult") {
            parseIntegerProperty(value, accountInstance->lastAuthenticationResult);
         } else if (key == "StickyAccountId") {
            parseStringProperty(value, accountInstance->stickyAccountId);
         } else if (key == "NextAccountId") {
            parseStringProperty(value, accountInstance->nextAccountId);
         } else if (key == "StickyPrincipalId") {
            parseIntegerProperty(value, accountInstance->stickyPrincipalId);
         } else if (key == "IsServerAccountDeleted") {
            parseBoolProperty(value, accountInstance->isServerAccountDeleted);
         } else if (key == "ServerAccountStatus") {
            parseIntegerProperty(value, accountInstance->serverAccountStatus);
         } else if (key == "MiiImageLastModifiedDate") {
            parseStringProperty(value, accountInstance->miiImageLastModifiedDate);
         } else if (key == "IsCommitted") {
            parseBoolProperty(value, accountInstance->isCommitted);
         }
      });

   if (result == FSAStatus::OK) {
      return true;
   }

   std::memset(accountInstance.get(), 0, sizeof(AccountInstance));
   return false;
}

} // namespace ios::fpd::internal
