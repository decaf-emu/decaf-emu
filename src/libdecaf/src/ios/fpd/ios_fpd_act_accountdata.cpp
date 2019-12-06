#include "ios_fpd_act_accountdata.h"
#include "ios_fpd_act_server.h"
#include "ios_fpd_log.h"

#include "ios/fs/ios_fs_fsa_ipc.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_process.h"
#include "nn/ffl/nn_ffl_miidata.h"
#include "nn/act/nn_act_result.h"

#include <common/platform_time.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <common/platform_time.h>
#include <cstring>
#include <string_view>

using namespace nn::act;
using namespace nn::ffl;
using namespace ios::fs;
using namespace ios::kernel;

namespace ios::fpd::internal
{

//! TODO: This hash is actually read from /dev/mcp ioctl 0xD3
static constexpr std::array<uint8_t, 4> UnknownHash = { 0xDE, 0xCA, 0xF0, 0x0D };

//! TODO: Device Hash is actually calculated from UnkownHash using some sort of sha256
static constexpr std::array<uint8_t, 8> DeviceHash = { 0xDE, 0xCA, 0xFD, 0xEC, 0xAF, 0xDE, 0xCA, 0xF0 };

struct AccountData
{
   be2_struct<UuidManager> uuidManager;
   be2_struct<TransferableIdManager> transferableIdManager;
   be2_struct<PersistentIdManager> persistentIdManager;
   be2_struct<AccountManager> accountManager;
   be2_array<AccountInstance, NumSlots> accounts;
   be2_array<bool, NumSlots> accountUsed;

   be2_phys_ptr<AccountInstance> currentAccount;
   be2_phys_ptr<AccountInstance> defaultAccount;
};

static phys_ptr<AccountData> sAccountData = nullptr;

phys_ptr<TransferableIdManager>
getTransferableIdManager()
{
   return phys_addrof(sAccountData->transferableIdManager);
}

phys_ptr<UuidManager>
getUuidManager()
{
   return phys_addrof(sAccountData->uuidManager);
}

phys_ptr<PersistentIdManager>
getPersistentIdManager()
{
   return phys_addrof(sAccountData->persistentIdManager);
}

phys_ptr<AccountManager>
getAccountManager()
{
   return phys_addrof(sAccountData->accountManager);
}

phys_ptr<AccountInstance>
getCurrentAccount()
{
   return sAccountData->currentAccount;
}

phys_ptr<AccountInstance>
getDefaultAccount()
{
   return sAccountData->defaultAccount;
}

std::array<uint8_t, 8>
getDeviceHash()
{
   return DeviceHash;
}

uint8_t
getNumAccounts()
{
   auto count = uint8_t { 0 };
   for (auto used : sAccountData->accountUsed) {
      if (used) {
         count++;
      }
   }

   return count;
}

SlotNo
getSlotNoForAccount(phys_ptr<AccountInstance> account)
{
   auto index = account - phys_addrof(sAccountData->accounts[0]);
   if (index < 0 || index >= sAccountData->accounts.size()) {
      return InvalidSlot;
   }

   return static_cast<SlotNo>(index + 1);
}

phys_ptr<AccountInstance>
getAccountBySlotNo(SlotNo slot)
{
   if (slot == CurrentUserSlot) {
      return sAccountData->currentAccount;
   } else if (slot == InvalidSlot) {
      return nullptr;
   }

   auto index = static_cast<unsigned>(slot - 1);
   if (index >= sAccountData->accounts.size()) {
      return nullptr;
   }

   return phys_addrof(sAccountData->accounts[index]);
}

phys_ptr<AccountInstance>
getAccountByPersistentId(PersistentId id)
{
   for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
      if (sAccountData->accountUsed[i] &&
          sAccountData->accounts[i].persistentId == id) {
         return phys_addrof(sAccountData->accounts[i]);
      }
   }

   return nullptr;
}

static uint64_t
sub_E30BD3A0(uint32_t a, uint32_t b)
{
   uint32_t v2 = (b << 8) & 0xFF0000 | (b << 24) | (b >> 8) & 0xFF00 | (b >> 24);
   uint32_t v3 = (a << 8) & 0xFF0000 | (a << 24) | (a >> 8) & 0xFF00 | (a >> 24);
   uint32_t v4 = v2 ^ (v2 ^ (v2 >> 7)) & 0xAA00AA ^ (((v2 ^ (v2 >> 7)) & 0xAA00AA) << 7);
   uint32_t v5 = v3 ^ (v3 ^ (v3 >> 7)) & 0xAA00AA ^ (((v3 ^ (v3 >> 7)) & 0xAA00AA) << 7);
   uint32_t v6 = v5 ^ (v5 ^ (v5 >> 14)) & 0xCCCC ^ (((v5 ^ (v5 >> 14)) & 0xCCCC) << 14);
   uint32_t v7 = v4 ^ (v4 ^ (v4 >> 14)) & 0xCCCC ^ (((v4 ^ (v4 >> 14)) & 0xCCCC) << 14);

   uint32_t resultLo =
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) << 8) & 0xFF0000 |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) << 24) |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) >> 8) & 0xFF00 |
      ((v7 & 0xF0F0F0F0 | (v6 >> 4) & 0xF0F0F0F) >> 24);
   uint32_t resultHi =
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) << 8) & 0xFF0000 |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) << 24) |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) >> 8) & 0xFF00 |
      ((16 * v7 & 0xF0F0F0F0 | v6 & 0xF0F0F0F) >> 24);

   return static_cast<uint64_t>(resultLo) | (static_cast<uint64_t>(resultHi) << 32);
}

TransferrableId
calculateTransferableId(uint64_t transferableIdBase, uint16_t a3)
{
   uint32_t a1_hi = static_cast<uint32_t>(transferableIdBase >> 32);
   uint32_t a1_lo = static_cast<uint32_t>(transferableIdBase);
   uint32_t v3 = (a3 << 6) | (a1_hi);
   uint32_t v4 = ((v3 >> 16) & 0x1C0) | ((a1_lo << 3) & 0x38) | ((v3 >> 6) & 7);
   return sub_E30BD3A0(
      v3 ^ (((v4 << 31) | (v4 << 22) | (v4 >> 5) | (v4 >> 23) | (v4 >> 14) | (v4 << 4) | (v4 << 13)) & 0xFFFFFE3F),
      a1_lo ^ ((v4 << 27) | (v4 << 9) | (v4 << 18) | v4));
}

static uint16_t
FFLiGetCRC16(phys_ptr<const uint8_t> bytes,
             uint32_t length)
{
   auto crc = uint32_t { 0 };

   for (auto byteIndex = 0u; byteIndex < length; byteIndex++) {
      for (auto bitIndex = 7; bitIndex >= 0; bitIndex--) {
         crc = (((crc << 1) | ((bytes[byteIndex] >> bitIndex) & 0x1)) ^
               (((crc & 0x8000) != 0) ? 0x1021 : 0));
      }
   }

   for (auto counter = 16; counter > 0u; counter--) {
      crc = ((crc << 1) ^ (((crc & 0x8000) != 0) ? 0x1021 : 0));
   }

   return static_cast<uint16_t>(crc & 0xFFFF);
}

static void
FFLiSetAuthorID(uint64_t *authorId)
{
   *authorId = calculateTransferableId(
      sAccountData->accountManager.commonTransferableIdBase, 0x4A0);
}

static void
FFLiSetCreateID(FFLCreateID *createId)
{
   static constexpr tm MiiEpoch = { 0, 0, 0, 1, 0, 2010 - 1900, 0, 0, 0 };
   auto epoch = std::chrono::system_clock::from_time_t(platform::make_gm_time(MiiEpoch));
   auto secondsSinceMiiEpoch =
      std::chrono::duration_cast<std::chrono::seconds>(
         std::chrono::system_clock::now() - epoch);

   createId->flags = FFLCreateIDFlags::IsNormalMii | FFLCreateIDFlags::IsWiiUMii;
   createId->timestamp = static_cast<uint32_t>(secondsSinceMiiEpoch.count() & 0x0FFFFFFF);
   std::memcpy(createId->deviceHash, DeviceHash.data(), 6);
}

static long long
getUuidTime()
{
   auto start = std::chrono::system_clock::from_time_t(-12219292800);
   auto diff = std::chrono::system_clock::now() - start;
   auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff).count();
   return ns / 100;
}

static std::array<uint8_t, UuidSize>
generateUuid()
{
   auto time = getUuidTime();
   sAccountData->uuidManager.lastTime = time;

   auto time_low = static_cast<uint32_t>(time);
   auto time_mid = static_cast<uint16_t>((time >> 32) & 0xFFFF);
   auto time_hi_and_version = static_cast<uint16_t>(
      static_cast<uint16_t>((time >> 48) & 0x0FFF) | 0x1000);

   auto clock_seq = static_cast<uint16_t>(
      (++sAccountData->uuidManager.clockSequence & 0x3FFF) | 0x8000);
   if (sAccountData->uuidManager.clockSequence >= 0x4000) {
      sAccountData->uuidManager.clockSequence = 0;
   }

   auto node = std::array<uint8_t, 6> { };
   node.fill(0);
   node[0] = 1;
   node[1] = 1;
   std::memcpy(node.data() + 2, UnknownHash.data(), UnknownHash.size());

   auto uuid = std::array<uint8_t, UuidSize> { };
   std::memcpy(uuid.data() + 0, &time_low, 4);
   std::memcpy(uuid.data() + 4, &time_mid, 2);
   std::memcpy(uuid.data() + 6, &time_hi_and_version, 2);
   std::memcpy(uuid.data() + 8, &clock_seq, 2);
   std::memcpy(uuid.data() + 10, node.data(), 6);
   return uuid;
}

static PersistentId
generatePersistentId()
{
   return ++sAccountData->persistentIdManager.persistentIdHead;
}

static TransferrableId
generateTransferrableIdBase()
{
   auto valueLo = *reinterpret_cast<const uint32_t *>(UnknownHash.data());
   auto valueHi = (sAccountData->transferableIdManager.counter << 22) | 4;
   sAccountData->transferableIdManager.counter =
      static_cast<uint32_t>((sAccountData->transferableIdManager.counter + 1) & 0x3FF);

   return static_cast<uint64_t>(valueLo) | (static_cast<uint64_t>(valueHi) << 32);
}

static phys_ptr<AccountInstance>
allocateAccount()
{
   for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
      if (!sAccountData->accountUsed[i]) {
         sAccountData->accountUsed[i] = true;
         return phys_addrof(sAccountData->accounts[i]);
      }
   }

   return nullptr;
}

phys_ptr<AccountInstance>
createAccount()
{
   auto account = allocateAccount();
   if (!account) {
      return nullptr;
   }

   std::memset(account.get(), 0, sizeof(AccountInstance));
   account->persistentId = generatePersistentId();
   account->parentalControlSlotNo = uint8_t { 1u };
   account->principalId = 1u;
   account->simpleAddressId = 1u;
   account->transferableIdBase = generateTransferrableIdBase();
   account->accountId = "DonaldTrump420";
   account->nfsPassword = "NfsPassword";
   account->birthDay = uint8_t { 4 };
   account->birthMonth = uint8_t { 6 };
   account->birthYear = uint16_t { 1989 };
   account->gender = uint8_t { 1 };
   account->uuid = generateUuid();
   account->isCommitted = uint8_t { 0 };

   // Default Mii from IOS
   static uint8_t DefaultMii[] = {
      0x00, 0x01, 0x00, 0x40, 0x80, 0xF3, 0x41, 0x80, 0x02, 0x65, 0xA0, 0x92,
      0xD2, 0x3B, 0x13, 0x36, 0xA4, 0xC0, 0xE1, 0xF8, 0x2D, 0x06, 0x00, 0x00,
      0x00, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40,
      0x00, 0x00, 0x21, 0x01, 0x02, 0x68, 0x44, 0x18, 0x26, 0x34, 0x46, 0x14,
      0x81, 0x12, 0x17, 0x68, 0x0D, 0x00, 0x00, 0x29, 0x00, 0x52, 0x48, 0x50,
      0x00, 0x00, 0x61, 0x00, 0x72, 0x00, 0x61, 0x00, 0x68, 0x00, 0x00, 0x00,
      0x67, 0x00, 0x68, 0x00, 0x69, 0x00, 0x6A, 0x00, 0x00, 0x00, 0xE4, 0x62,
   };

   static_assert(sizeof(DefaultMii) == sizeof(account->miiData));
   std::memcpy(std::addressof(account->miiData), DefaultMii, sizeof(DefaultMii));
   FFLiSetAuthorID(&account->miiData.author_id);
   FFLiSetCreateID(&account->miiData.mii_id);
   account->miiData.checksum = FFLiGetCRC16(phys_cast<uint8_t *>(phys_addrof(account->miiData)),
      sizeof(FFLStoreData) - 2);
   account->miiName = u"???";

   return account;
}

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

static void
parseBoolProperty(std::string_view value, be2_val<uint8_t> &out)
{
   int temporary = 0;
   std::from_chars(value.data(), value.data() + value.size(), temporary, 16);
   out = temporary != 0;
}

template<typename Type>
static void
parseIntegerProperty(std::string_view value, be2_val<Type> &out)
{
   Type result { 0 };
   std::from_chars(value.data(), value.data() + value.size(), result, 16);
   out = result;
}

static void
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
static void
parseStringProperty(std::string_view value, be2_array<char, Size> &out)
{
   out.fill(0);
   std::memcpy(phys_addrof(out).get(), value.data(),
               std::min<std::size_t>(value.size(), out.size()));

   if (value.size() == out.size()) {
      out.back() = '\0';
   }
}

static bool
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

static bool
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

static bool
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

static bool
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

   accountManager->defaultNnasNfsEnv.fill(0);
   accountManager->defaultNnasNfsEnv = "L1";
   parseNnasNfsEnv(phys_addrof(accountManager->defaultNnasNfsEnv).get(),
                   accountManager->defaultNfsType,
                   accountManager->defaultNfsNo);
   return false;
}

static bool
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
            parseHexString(value, phys_addrof(accountInstance->uuid),
                           accountInstance->uuid.size());
         } else if (key == "ParentalControlSlotNo") {
            parseIntegerProperty(value, accountInstance->parentalControlSlotNo);
         } else if (key == "MiiData") {
            parseHexString(value,
                           phys_cast<uint8_t *>(phys_addrof(accountInstance->miiData)),
                           sizeof(accountInstance->miiData));
         } else if (key == "MiiName") {
            parseHexString(value,
                           phys_cast<uint8_t *>(phys_addrof(accountInstance->miiName)),
                           accountInstance->miiName.size() * 2);
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
            parseHexString(value, phys_addrof(accountInstance->accountPasswordHash),
                           accountInstance->accountPasswordHash.size());
         } else if (key == "IsPasswordCacheEnabled") {
            parseBoolProperty(value, accountInstance->isPasswordCacheEnabled);
         } else if (key == "AccountPasswordCache") {
            parseHexString(value, phys_addrof(accountInstance->accountPasswordCache),
                           accountInstance->accountPasswordCache.size());
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

void
initialiseAccounts()
{
   loadTransferableIdManager(phys_addrof(sAccountData->transferableIdManager));
   loadUuidManager(phys_addrof(sAccountData->uuidManager));
   loadPersistentIdManager(phys_addrof(sAccountData->persistentIdManager));

   loadAccountManager(phys_addrof(sAccountData->accountManager));
   if (!sAccountData->accountManager.commonUuid[0]) {
      sAccountData->accountManager.commonUuid = generateUuid();
   }

   if (!sAccountData->accountManager.commonTransferableIdBase) {
      sAccountData->accountManager.commonTransferableIdBase = generateTransferrableIdBase();
   }

   for (auto slot = 1u; slot <= sAccountData->accountManager.persistentIdList.size(); ++slot) {
      auto persistentId = sAccountData->accountManager.persistentIdList[slot - 1];
      if (persistentId) {
         sAccountData->accountUsed[slot - 1] =
            loadAccountInstance(phys_addrof(sAccountData->accounts[slot - 1]),
                                persistentId);
      }
   }

   // Try find the default account
   auto account = getAccountByPersistentId(sAccountData->accountManager.defaultAccountPersistentId);
   if (!account) {
      // Try find any account
      for (auto i = 0u; i < sAccountData->accounts.size(); ++i) {
         if (sAccountData->accountUsed[i]) {
            account = phys_addrof(sAccountData->accounts[i]);
            break;
         }
      }

      if (!account) {
         // No account found, we can create one instead!
         account = createAccount();
         account->isCommitted = uint8_t { 1u };
         sAccountData->accountManager.persistentIdList[getSlotNoForAccount(account)] = account->persistentId;
         sAccountData->accountManager.defaultAccountPersistentId = account->persistentId;
      }
   }

   sAccountData->currentAccount = account;
   sAccountData->defaultAccount = account;
}

void
initialiseStaticAccountData()
{
   sAccountData = kernel::allocProcessStatic<AccountData>();
}

} // namespace ios::fpd::internal
