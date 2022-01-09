#include "ios_crypto.h"
#include "ios_crypto_enum.h"
#include "ios_crypto_log.h"
#include "ios_crypto_request.h"

#include "decaf_log.h"
#include "ios/ios_stackobject.h"
#include "ios/kernel/ios_kernel_debug.h"
#include "ios/kernel/ios_kernel_hardware.h"
#include "ios/kernel/ios_kernel_heap.h"
#include "ios/kernel/ios_kernel_otp.h"
#include "ios/kernel/ios_kernel_resourcemanager.h"
#include "ios/mcp/ios_mcp_ipc.h"

#include <gsl/gsl-lite.hpp>
#include <openssl/err.h>
#include <openssl/evp.h>

using namespace ios::kernel;
using namespace ios::mcp;

namespace ios::crypto
{

constexpr auto kCryptoHeapSize = 0x1000u;
constexpr auto kCrossHeapSize = 0x10000u;
constexpr auto kMainThreadNumMessages = 260u;

struct CryptoDevice
{
   be2_val<BOOL> used = FALSE;
   be2_val<ProcessId> processId;
};

struct OtpKeys
{
   be2_val<uint32_t> ngId;
   be2_array<uint8_t, OtpFieldSize::CommonKey> commonKey;
   be2_array<uint8_t, OtpFieldSize::DrhWlanKey> drhWlanKey;
   be2_array<uint8_t, OtpFieldSize::MlcKey> mlcKey;
   be2_array<uint8_t, OtpFieldSize::NgPrivateKey> ngPrivateKey;
   be2_array<uint8_t, OtpFieldSize::NssPrivateKey> nssPrivateKey;
   be2_array<uint8_t, OtpFieldSize::RngKey> rngKey;
   be2_array<uint8_t, OtpFieldSize::SeepromKey> seepromKey;
   be2_array<uint8_t, OtpFieldSize::SlcHmac> slcHmac;
   be2_array<uint8_t, OtpFieldSize::SlcKey> slcKey;
   be2_array<uint8_t, OtpFieldSize::SslRsaKey> sslRsaKey;
   be2_array<uint8_t, OtpFieldSize::StarbuckAncastKey> starbuckAncastKey;
   be2_array<uint8_t, OtpFieldSize::XorKey> xorKey;
   be2_array<uint8_t, OtpFieldSize::Unknown0x50> unknown0x50;

   be2_array<uint8_t, OtpFieldSize::WiiCommonKey> wiiCommonKey;
   be2_array<uint8_t, OtpFieldSize::WiiKoreanKey> wiiKoreanKey;
   be2_array<uint8_t, OtpFieldSize::WiiNandHmac> wiiNandHmac;
   be2_array<uint8_t, OtpFieldSize::WiiNandKey> wiiNandKey;
   be2_array<uint8_t, OtpFieldSize::WiiNssPrivateKey> wiiNssPrivateKey;

   be2_array<uint8_t, OtpFieldSize::VwiiCommonKey> vwiiCommonKey;
};

struct KeyData
{
   be2_val<BOOL> used = FALSE;
   be2_array<uint8_t, 32> data;
   be2_phys_ptr<KeyData> next = nullptr;
};

struct KeyHandle
{
   be2_val<BOOL> used = FALSE;
   be2_val<ObjectType> objectType;
   be2_val<SubObjectType> subObjectType;
   be2_phys_ptr<KeyData> data;
   be2_val<uint32_t> permission;
   be2_val<uint32_t> idData;
};

struct StaticCryptoData
{
   be2_val<BOOL> initialised = FALSE;
   be2_val<HeapId> cryptoHeapId = -1;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, kMainThreadNumMessages> messages;
   be2_array<CryptoDevice, 32> handles;
   be2_struct<OtpKeys> otpKeys;

   be2_array<KeyData, 256> keyDataList;
   be2_array<KeyHandle, 128> keyHandleList;
};

static phys_ptr<StaticCryptoData> sCryptoData = nullptr;
static phys_ptr<void> sCryptoHeap = nullptr;


namespace internal
{

Logger cryptoLog = { };

static void
initialiseStaticData()
{
   sCryptoData = allocProcessStatic<StaticCryptoData>();
   sCryptoHeap = allocProcessLocalHeap(kCryptoHeapSize);
}

static phys_ptr<CryptoDevice>
getCryptoDevice(uint32_t handle)
{
   if (handle < 0 || handle >= sCryptoData->handles.size()) {
      return nullptr;
   }

   if (!sCryptoData->handles[handle].used) {
      return nullptr;
   }

   return phys_addrof(sCryptoData->handles[handle]);
}

static phys_ptr<KeyHandle>
getCryptoKey(uint32_t handle)
{
   if (handle < 0 || handle >= sCryptoData->keyHandleList.size()) {
      return nullptr;
   }

   if (!sCryptoData->keyHandleList[handle].used) {
      return nullptr;
   }

   return phys_addrof(sCryptoData->keyHandleList[handle]);
}

static phys_ptr<KeyData>
allocateKeyData(uint32_t size)
{
   phys_ptr<KeyData> firstData = nullptr;
   phys_ptr<KeyData> prevData = nullptr;

   for (auto i = 0u; size > 0 && i < sCryptoData->keyDataList.size(); ++i) {
      if (sCryptoData->keyDataList[i].used) {
         continue;
      }

      if (size < sCryptoData->keyDataList[i].data.size()) {
         size = 0;
      } else {
         size -= sCryptoData->keyDataList[i].data.size();
      }

      sCryptoData->keyDataList[i].used = TRUE;

      if (prevData) {
         prevData->next = phys_addrof(sCryptoData->keyDataList[i]);
      }
      prevData = phys_addrof(sCryptoData->keyDataList[i]);
      if (!firstData) {
         firstData = prevData;
      }
   }

   return firstData;
}

static Error
getKeyObjectSize(ObjectType objectType,
                 SubObjectType subObjectType,
                 uint32_t *outKeySize)
{
   struct ObjectSize {
      ObjectType objectType;
      SubObjectType subObjectType;
      uint32_t size;
   };
   static const ObjectSize kObjectSizes[] = {
      { ObjectType::SecretKey,   SubObjectType::Aes128,        16 },
      { ObjectType::SecretKey,   SubObjectType::Mac,           20 },
      { ObjectType::SecretKey,   SubObjectType::Ecc233,        30 },
      { ObjectType::SecretKey,   SubObjectType::Unknown0x7,    64 },
      { ObjectType::PublicKey,   SubObjectType::Rsa2048,       256 },
      { ObjectType::PublicKey,   SubObjectType::Rsa4096,       512 },
      { ObjectType::PublicKey,   SubObjectType::Ecc233,        60 },
      { ObjectType::Unknown0x2,  SubObjectType::Ecc233,        90 },
      { ObjectType::Data,        SubObjectType::Data,          0 },
      { ObjectType::Data,        SubObjectType::Version,       0 },
   };

   for (const auto &objectSize : kObjectSizes) {
      if (objectSize.objectType == objectType &&
          objectSize.subObjectType == subObjectType) {
         *outKeySize = objectSize.size;
         return Error::OK;
      }
   }

   return static_cast<Error>(-2005);
}

static Error
registerSystemKey(KeyId keyId,
                  ObjectType objectType,
                  SubObjectType subObjectType,
                  phys_ptr<void> data,
                  uint32_t size,
                  phys_ptr<uint32_t> idData)
{
   Error error;
   uint32_t expectedKeySize = 0;

   error = getKeyObjectSize(objectType, subObjectType, &expectedKeySize);
   if (error != Error::OK) {
      return error;
   }

   if (size != expectedKeySize) {
      return static_cast<Error>(-2014);
   }

   phys_ptr<KeyData> keyData = allocateKeyData(size);
   if (size > 0 && !keyData) {
      return static_cast<Error>(-2013);
   }

   phys_ptr<KeyHandle> keyHandle =
      phys_addrof(sCryptoData->keyHandleList[static_cast<uint32_t>(keyId)]);
   if (keyHandle->used) {
      return static_cast<Error>(-2001);
   }

   keyHandle->used = TRUE;
   keyHandle->objectType = objectType;
   keyHandle->subObjectType = subObjectType;
   keyHandle->data = keyData;
   keyHandle->permission = 0u;
   keyHandle->idData = idData ? static_cast<uint32_t>(*idData) : 0u;

   uint32_t bytesCopied = 0;
   while (bytesCopied < size) {
      uint32_t copySize = std::min(size - bytesCopied, keyData->data.size());
      memcpy(std::addressof(keyData->data),
             static_cast<uint8_t *>(data.get()) + bytesCopied,
             copySize);
      bytesCopied += copySize;
      keyData = keyData->next;
   }

   return Error::OK;
}

static Error
setSystemKeyPermission(KeyId keyId,
                       uint32_t permission)
{
   phys_ptr<KeyHandle> keyHandle =
      phys_addrof(sCryptoData->keyHandleList[static_cast<uint32_t>(keyId)]);
   if (!keyHandle->used) {
      return static_cast<Error>(-2002);
   }

   keyHandle->permission = permission;
   return Error::OK;
}

static Error
readOtpKeys()
{
   StackArray<uint8_t, 64> buffer;

   IOS_ReadOTP(OtpFieldIndex::NgId,
               phys_addrof(sCryptoData->otpKeys.ngId),
               sizeof(sCryptoData->otpKeys.ngId));
   IOS_ReadOTP(OtpFieldIndex::NgPrivateKey,
               phys_addrof(sCryptoData->otpKeys.ngPrivateKey),
               sizeof(sCryptoData->otpKeys.ngPrivateKey));
   IOS_ReadOTP(OtpFieldIndex::NssPrivateKey,
               phys_addrof(sCryptoData->otpKeys.nssPrivateKey),
               sizeof(sCryptoData->otpKeys.nssPrivateKey));

   IOS_ReadOTP(OtpFieldIndex::WiiNssPrivateKey,
               phys_addrof(sCryptoData->otpKeys.wiiNssPrivateKey),
               sizeof(sCryptoData->otpKeys.wiiNssPrivateKey));
   IOS_ReadOTP(OtpFieldIndex::WiiCommonKey,
               phys_addrof(sCryptoData->otpKeys.wiiCommonKey),
               sizeof(sCryptoData->otpKeys.wiiCommonKey));

   if (sCryptoData->otpKeys.ngId != 0) {
      IOS_ReadOTP(OtpFieldIndex::WiiNandHmac,
                  phys_addrof(sCryptoData->otpKeys.wiiNandHmac),
                  sizeof(sCryptoData->otpKeys.wiiNandHmac));
      IOS_ReadOTP(OtpFieldIndex::WiiNandKey,
                  phys_addrof(sCryptoData->otpKeys.wiiNandKey),
                  sizeof(sCryptoData->otpKeys.wiiNandKey));
   }

   IOS_ReadOTP(OtpFieldIndex::SlcKey,
               phys_addrof(sCryptoData->otpKeys.slcKey),
               sizeof(sCryptoData->otpKeys.slcKey));
   IOS_ReadOTP(OtpFieldIndex::SlcHmac,
               phys_addrof(sCryptoData->otpKeys.slcHmac),
               sizeof(sCryptoData->otpKeys.slcHmac));
   IOS_ReadOTP(OtpFieldIndex::RngKey,
               phys_addrof(sCryptoData->otpKeys.rngKey),
               sizeof(sCryptoData->otpKeys.rngKey));
   IOS_ReadOTP(OtpFieldIndex::StarbuckAncastKey,
               phys_addrof(sCryptoData->otpKeys.starbuckAncastKey),
               sizeof(sCryptoData->otpKeys.starbuckAncastKey));
   IOS_ReadOTP(OtpFieldIndex::SeepromKey,
               phys_addrof(sCryptoData->otpKeys.seepromKey),
               sizeof(sCryptoData->otpKeys.seepromKey));
   IOS_ReadOTP(OtpFieldIndex::MlcKey,
               phys_addrof(sCryptoData->otpKeys.mlcKey),
               sizeof(sCryptoData->otpKeys.mlcKey));
   IOS_ReadOTP(OtpFieldIndex::DrhWlanKey,
               phys_addrof(sCryptoData->otpKeys.drhWlanKey),
               sizeof(sCryptoData->otpKeys.drhWlanKey));
   IOS_ReadOTP(OtpFieldIndex::VwiiCommonKey,
               phys_addrof(sCryptoData->otpKeys.vwiiCommonKey),
               sizeof(sCryptoData->otpKeys.vwiiCommonKey));
   IOS_ReadOTP(OtpFieldIndex::CommonKey,
               phys_addrof(sCryptoData->otpKeys.commonKey),
               sizeof(sCryptoData->otpKeys.commonKey));
   IOS_ReadOTP(OtpFieldIndex::WiiKoreanKey,
               phys_addrof(sCryptoData->otpKeys.wiiKoreanKey),
               sizeof(sCryptoData->otpKeys.wiiKoreanKey));
   IOS_ReadOTP(OtpFieldIndex::SslRsaKey,
               phys_addrof(sCryptoData->otpKeys.sslRsaKey),
               sizeof(sCryptoData->otpKeys.sslRsaKey));
   IOS_ReadOTP(OtpFieldIndex::NssPrivateKey,
               phys_addrof(sCryptoData->otpKeys.nssPrivateKey),
               sizeof(sCryptoData->otpKeys.nssPrivateKey));
   IOS_ReadOTP(OtpFieldIndex::XorKey,
               phys_addrof(sCryptoData->otpKeys.xorKey),
               sizeof(sCryptoData->otpKeys.xorKey));
   IOS_ReadOTP(OtpFieldIndex::Unknown0x50,
               phys_addrof(sCryptoData->otpKeys.unknown0x50),
               sizeof(sCryptoData->otpKeys.unknown0x50));

   registerSystemKey(KeyId::NgId,
                     ObjectType::Data, SubObjectType::Data,
                     nullptr, 0,
                     phys_addrof(sCryptoData->otpKeys.ngId));
   registerSystemKey(KeyId::NgPrivateKey,
                     ObjectType::SecretKey, SubObjectType::Ecc233,
                     phys_addrof(sCryptoData->otpKeys.ngPrivateKey), 30,
                     nullptr);
   registerSystemKey(KeyId::NssPrivateKey,
                     ObjectType::SecretKey, SubObjectType::Ecc233,
                     phys_addrof(sCryptoData->otpKeys.nssPrivateKey), 30,
                     nullptr);
   registerSystemKey(KeyId::WiiNssPrivateKey,
                     ObjectType::SecretKey, SubObjectType::Ecc233,
                     phys_addrof(sCryptoData->otpKeys.wiiNssPrivateKey), 30,
                     nullptr);
   registerSystemKey(KeyId::SlcKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.slcKey), 16,
                     nullptr);
   registerSystemKey(KeyId::SlcHmac,
                     ObjectType::SecretKey, SubObjectType::Mac,
                     phys_addrof(sCryptoData->otpKeys.slcHmac), 20,
                     nullptr);
   registerSystemKey(KeyId::WiiCommonKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.wiiCommonKey), 16,
                     nullptr);
   registerSystemKey(KeyId::RngKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.rngKey), 16,
                     nullptr);

   static std::array<uint8_t, 16> wiiSdKey = {
      0xab, 0x01, 0xb9, 0xd8, 0xe1, 0x62, 0x2b, 0x08,
      0xaf, 0xba, 0xd8, 0x4d, 0xbf, 0xc2, 0xa5, 0x5d,
   };
   memcpy(buffer.get(), wiiSdKey.data(), wiiSdKey.size());
   registerSystemKey(KeyId::WiiSdKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);

   registerSystemKey(KeyId::WiiKoreanKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.wiiKoreanKey), 16,
                     nullptr);
   registerSystemKey(KeyId::StarbuckAncastKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.starbuckAncastKey), 16,
                     nullptr);
   registerSystemKey(KeyId::CommonKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.commonKey), 16,
                     nullptr);
   registerSystemKey(KeyId::VwiiCommonKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.vwiiCommonKey), 16,
                     nullptr);
   registerSystemKey(KeyId::WiiNandKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.wiiNandKey), 16,
                     nullptr);
   registerSystemKey(KeyId::WiiNandHmac,
                     ObjectType::SecretKey, SubObjectType::Mac,
                     phys_addrof(sCryptoData->otpKeys.wiiNandHmac), 20,
                     nullptr);

   /*
   registerSystemKey(KeyId::StarbuckAncastModulus,
                     ObjectType::PublicKey, SubObjectType::Rsa2048,
                     data, 0x100, id);
   registerSystemKey(KeyId::Boot1AncastModulus,
                     ObjectType::PublicKey, SubObjectType::Rsa2048,
                     data, 0x100, id);
   */

   registerSystemKey(KeyId::SeepromKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.seepromKey), 16,
                     nullptr);
   registerSystemKey(KeyId::MlcKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.mlcKey), 16,
                     nullptr);
   registerSystemKey(KeyId::DrhWlanKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.drhWlanKey), 16,
                     nullptr);
   registerSystemKey(KeyId::Unknown0x1A,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.unknown0x50), 16,
                     nullptr);
   registerSystemKey(KeyId::SslRsaKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.sslRsaKey), 16,
                     nullptr);
   registerSystemKey(KeyId::NssPrivateKey2,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     phys_addrof(sCryptoData->otpKeys.nssPrivateKey), 16,
                     nullptr);

   static std::array<uint8_t, 16> spotpassXorKey = {
      0x33, 0xAC, 0x6D, 0x15, 0xC2, 0x26, 0x0A, 0x91,
      0x3B, 0xBF, 0x73, 0xC3, 0x55, 0xD8, 0x66, 0x04,
   };
   for (size_t i = 0u; i < spotpassXorKey.size(); ++i) {
      buffer[i] =
         spotpassXorKey[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::SpotPassKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);

   static std::array<uint8_t, 64> spotpassXorUnknown0x20 = {
      0x7F, 0xA5, 0x48, 0x15, 0xD3, 0x6E, 0x4E, 0xBC,
      0x38, 0x87, 0x6A, 0x82, 0x16, 0xF2, 0x59, 0x24,
      0x39, 0x98, 0x0F, 0x13, 0xD8, 0x26, 0x0E, 0xA7,
      0x3B, 0x94, 0x42, 0xCB, 0x41, 0xFB, 0x7C, 0x3C,
      0x45, 0xE9, 0x6F, 0x12, 0xAA, 0x39, 0x5D, 0x9C,
      0x42, 0x9A, 0x47, 0xC6, 0x49, 0xF7, 0x61, 0x25,
      0x69, 0x9F, 0x0E, 0x62, 0xE3, 0x6E, 0x06, 0xF8,
      0x59, 0xA6, 0x4C, 0xB0, 0x41, 0xCF, 0x26, 0x48,
   };
   for (size_t i = 0u; i < spotpassXorUnknown0x20.size(); ++i) {
      buffer[i] =
         spotpassXorUnknown0x20[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::SpotPassUnknown0x20,
                     ObjectType::SecretKey, SubObjectType::Unknown0x7,
                     buffer, 64,
                     nullptr);

   // This key is 64 bytes with first 32 bytes of data and last 32 bytes zero
   static std::array<uint8_t, 32> xorUnknown0x21 = {
      0x24, 0xF0, 0x31, 0xEE, 0x47, 0x4B, 0xCE, 0x34,
      0x80, 0x18, 0x1B, 0x0F, 0x11, 0xDB, 0xE5, 0xC6,
      0x69, 0x16, 0x77, 0xE8, 0x89, 0x89, 0x2E, 0x62,
      0x61, 0xBE, 0xE4, 0xDC, 0x46, 0xD7, 0x3C, 0x30,
   };
   for (size_t i = 0u; i < xorUnknown0x21.size(); ++i) {
      buffer[i] =
         xorUnknown0x21[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   memset(buffer.get() + 32, 0, 32);
   registerSystemKey(KeyId::Unknown0x21,
                     ObjectType::SecretKey, SubObjectType::Unknown0x7,
                     buffer, 64,
                     nullptr);

   setSystemKeyPermission(KeyId::NgId, 0xfffffff);
   setSystemKeyPermission(KeyId::NgPrivateKey, 2);
   setSystemKeyPermission(KeyId::NssPrivateKey, 0x200);
   setSystemKeyPermission(KeyId::WiiNssPrivateKey, 0x200);
   setSystemKeyPermission(KeyId::SlcKey, 0x20);
   setSystemKeyPermission(KeyId::SlcHmac, 0x20);
   setSystemKeyPermission(KeyId::WiiCommonKey, 2);
   setSystemKeyPermission(KeyId::RngKey, 2);
   setSystemKeyPermission(KeyId::WiiSdKey, 0xfffffff);
   setSystemKeyPermission(KeyId::WiiKoreanKey, 2);
   setSystemKeyPermission(KeyId::CommonKey, 2);
   setSystemKeyPermission(KeyId::VwiiCommonKey, 2);
   setSystemKeyPermission(KeyId::WiiNandKey, 0x20);
   setSystemKeyPermission(KeyId::WiiNandHmac, 0x20);
   setSystemKeyPermission(KeyId::StarbuckAncastKey, 2);
   setSystemKeyPermission(KeyId::StarbuckAncastModulus, 2);
   setSystemKeyPermission(KeyId::Boot1AncastModulus, 2);
   setSystemKeyPermission(KeyId::SeepromKey, 2);
   setSystemKeyPermission(KeyId::MlcKey, 0x20);
   setSystemKeyPermission(KeyId::DrhWlanKey, 0x40);
   setSystemKeyPermission(KeyId::Unknown0x1A, 0x20);
   setSystemKeyPermission(KeyId::SslRsaKey, 0x200);
   setSystemKeyPermission(KeyId::NssPrivateKey2, 0x200);

   static std::array<uint8_t, 16> udsLocalWlanCcmpKey = {
      0x34, 0x0a, 0xcf, 0xef, 0xb6, 0x95, 0x42, 0x9d,
      0x69, 0xae, 0x09, 0x44, 0xec, 0x37, 0x38, 0x7d,
   };
   for (size_t i = 0u; i < udsLocalWlanCcmpKey.size(); ++i) {
      buffer[i] =
         udsLocalWlanCcmpKey[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::UdsLocalWlanCcmpKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);

   static std::array<uint8_t, 16> dlpKey = {
      0x83, 0x7b, 0xe4, 0x1b, 0xcc, 0xb6, 0x5e, 0xaa,
      0x77, 0x29, 0x4c, 0x7f, 0x1e, 0xee, 0xef, 0xdc,
   };
   for (size_t i = 0u; i < dlpKey.size(); ++i) {
      buffer[i] =
         dlpKey[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::DlpKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);

   static std::array<uint8_t, 16> aptWrapKey = {
      0x53, 0x20, 0xbb, 0x5e, 0xfe, 0x10, 0xd4, 0xa8,
      0x9c, 0xca, 0x72, 0xd3, 0xcd, 0x3f, 0xd8, 0x22,
   };
   for (size_t i = 0u; i < aptWrapKey.size(); ++i) {
      buffer[i] =
         aptWrapKey[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::AptWrapKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);

   setSystemKeyPermission(KeyId::AptWrapKey, 0xfffffff);
   setSystemKeyPermission(KeyId::SpotPassKey, 0x800);
   setSystemKeyPermission(KeyId::SpotPassUnknown0x20, 0x800);
   setSystemKeyPermission(KeyId::Unknown0x21, 0x800);

   static std::array<uint8_t, 16> pushmoreKey = {
      0x3b, 0x66, 0x3d, 0x64, 0x3d, 0xd5, 0x3f, 0xc6,
      0x7b, 0xc4, 0xb7, 0x39, 0x8e, 0x23, 0x80, 0x92,
   };
   for (size_t i = 0u; i < pushmoreKey.size(); ++i) {
      buffer[i] =
         pushmoreKey[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::PushmoreKey,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);
   setSystemKeyPermission(KeyId::PushmoreKey, 0x800);

   // This key is 64 bytes with first 16 bytes of data and last 48 bytes zero
   static std::array<uint8_t, 16> unknown0x23 = {
      0x17, 0xca, 0x71, 0x17, 0xc1, 0x24, 0x9b, 0x9e,
      0x24, 0x47, 0x14, 0x97, 0x92, 0x21, 0xd4, 0x75,
   };
   for (size_t i = 0u; i < unknown0x23.size(); ++i) {
      buffer[i] =
         unknown0x23[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   memset(buffer.get() + 16, 0, 48);
   registerSystemKey(KeyId::Unknown0x23,
                     ObjectType::SecretKey, SubObjectType::Unknown0x7,
                     buffer, 64,
                     nullptr);
   setSystemKeyPermission(KeyId::Unknown0x23, 0x40);

   // This key is 64 bytes with first 16 bytes of data and last 48 bytes zero
   static std::array<uint8_t, 16> unknown0x22 = {
      0x75, 0xa9, 0x17, 0x08, 0xe9, 0xf4, 0x3e, 0xde,
      0xf2, 0x06, 0x55, 0xf6, 0x51, 0x12, 0x5d, 0x1d,
   };
   for (size_t i = 0u; i < unknown0x22.size(); ++i) {
      buffer[i] =
         unknown0x22[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   memset(buffer.get() + 16, 0, 48);
   registerSystemKey(KeyId::Unknown0x22,
                     ObjectType::SecretKey, SubObjectType::Unknown0x7,
                     buffer, 64,
                     nullptr);
   setSystemKeyPermission(KeyId::Unknown0x22, 0x40);

   static std::array<uint8_t, 16> unknown0x24 = {
      0xcb, 0xf7, 0x3d, 0x30, 0x4d, 0x7a, 0xd5, 0x94,
      0x4f, 0xb7, 0xbe, 0xb0, 0xc7, 0x48, 0xc4, 0x54,
   };
   for (size_t i = 0u; i < unknown0x24.size(); ++i) {
      buffer[i] =
         unknown0x24[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::Unknown0x24,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);
   setSystemKeyPermission(KeyId::Unknown0x24, 0x40);

   static std::array<uint8_t, 16> unknown0x25 = {
      0x5f, 0x02, 0x0b, 0x8e, 0x5b, 0x27, 0x0d, 0xee,
      0xe7, 0xc1, 0xb3, 0x49, 0xd8, 0xa7, 0x13, 0x0b,
   };
   for (size_t i = 0u; i < unknown0x25.size(); ++i) {
      buffer[i] =
         unknown0x25[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::Unknown0x25,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);
   setSystemKeyPermission(KeyId::Unknown0x25, 0x40);

   static std::array<uint8_t, 16> unknown0x26 = {
      0x2c, 0x77, 0x3e, 0x7a, 0xbd, 0xe9, 0xea, 0x54,
      0x69, 0x46, 0x76, 0x3b, 0xe8, 0x09, 0x89, 0xda,
   };
   for (size_t i = 0u; i < unknown0x26.size(); ++i) {
      buffer[i] =
         unknown0x26[i] ^
         sCryptoData->otpKeys.xorKey[i % sCryptoData->otpKeys.xorKey.size()];
   }
   registerSystemKey(KeyId::Unknown0x26,
                     ObjectType::SecretKey, SubObjectType::Aes128,
                     buffer, 16,
                     nullptr);
   setSystemKeyPermission(KeyId::Unknown0x26, 0x20);
   return Error::OK;
}

static Error
initialiseCrypto()
{
   if (sCryptoData->initialised) {
      return Error::OK;
   }

   Error error = IOS_CreateHeap(sCryptoHeap, kCryptoHeapSize);
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "initialiseCrypto: Failed to create crypto, error = {}.", error);
      return error;
   }

   sCryptoData->cryptoHeapId = static_cast<HeapId>(error);

   internal::cryptoLog->info("IOSC Initialize");
   readOtpKeys();

   sCryptoData->initialised = TRUE;
   return Error::OK;
}

static IOSCError
ioscDecrypt(phys_ptr<ResourceRequest> resourceRequest,
            phys_ptr<IOSCRequestDecrypt> decryptRequest,
            phys_ptr<void> iv, uint32_t ivSize,
            phys_ptr<void> input, uint32_t inputSize,
            phys_ptr<void> output, uint32_t outputSize)
{
   auto cryptoDevice = getCryptoDevice(resourceRequest->requestData.handle);
   if (!cryptoDevice) {
      return static_cast<IOSCError>(Error::Invalid);
   }

   if (cryptoDevice->processId > ProcessId::COSKERNEL) {
      return IOSCError::InvalidParam;
   }

   auto cryptoKey = getCryptoKey(decryptRequest->keyHandle);
   if (!cryptoKey) {
      return static_cast<IOSCError>(Error::Invalid);
   }

   if (!(cryptoKey->permission & (1 << static_cast<int>(cryptoDevice->processId)))) {
      return IOSCError::Permission;
   }

   EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
   auto _ = gsl::finally([&]() { EVP_CIPHER_CTX_free(ctx); });
   if (!EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL,
                           reinterpret_cast<unsigned char *>(cryptoKey->data.get()),
                           reinterpret_cast<unsigned char *>(iv.get()))) {
      return IOSCError::CryptoError;
   }

   EVP_CIPHER_CTX_set_key_length(ctx, 16);
   EVP_CIPHER_CTX_set_padding(ctx, 0);

   int updateLength = static_cast<int>(0);
   int finalLength = static_cast<int>(0);
   if (!EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(output.get()),
                          &updateLength,
                          reinterpret_cast<unsigned char *>(input.get()),
                          static_cast<int>(inputSize))) {
      return IOSCError::CryptoError;
   }

   if (!EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char *>(output.get()) + updateLength,
                            &finalLength)) {
      return IOSCError::CryptoError;
   }

   return IOSCError::OK;
}

static Error
cryptoIoctlv(phys_ptr<ResourceRequest> resourceRequest)
{
   auto error = Error::OK;

   switch (static_cast<IOSCCommand>(resourceRequest->requestData.args.ioctlv.request)) {
   case IOSCCommand::Decrypt:
   {
      if (resourceRequest->requestData.args.ioctlv.numVecIn != 3) {
         return Error::InvalidArg;
      }

      if (resourceRequest->requestData.args.ioctlv.numVecOut != 1) {
         return Error::InvalidArg;
      }

      if (!resourceRequest->requestData.args.ioctlv.vecs[0].paddr ||
           resourceRequest->requestData.args.ioctlv.vecs[0].len != sizeof(IOSCRequestDecrypt)) {
         return Error::InvalidArg;
      }

      if (!resourceRequest->requestData.args.ioctlv.vecs[1].paddr ||
           resourceRequest->requestData.args.ioctlv.vecs[1].len != 16) {
         return Error::InvalidArg;
      }

      if (!resourceRequest->requestData.args.ioctlv.vecs[2].paddr ||
          !resourceRequest->requestData.args.ioctlv.vecs[2].len) {
         return Error::InvalidArg;
      }

      if (!resourceRequest->requestData.args.ioctlv.vecs[3].paddr ||
          !resourceRequest->requestData.args.ioctlv.vecs[3].len) {
         return Error::InvalidArg;
      }

      auto decryptRequest = phys_cast<IOSCRequestDecrypt *>(
         resourceRequest->requestData.args.ioctlv.vecs[0].paddr);
      error = static_cast<Error>(
         ioscDecrypt(resourceRequest, decryptRequest,
            phys_cast<void *>(resourceRequest->requestData.args.ioctlv.vecs[1].paddr),
            resourceRequest->requestData.args.ioctlv.vecs[1].len,
            phys_cast<void *>(resourceRequest->requestData.args.ioctlv.vecs[2].paddr),
            resourceRequest->requestData.args.ioctlv.vecs[2].len,
            phys_cast<void *>(resourceRequest->requestData.args.ioctlv.vecs[3].paddr),
            resourceRequest->requestData.args.ioctlv.vecs[3].len));
      break;
   }
   default:
      error = Error::InvalidArg;
   }

   return error;
}

} // namespace internal

Error
processEntryPoint(phys_ptr<void> /* context */)
{
   StackObject<Message> message;

   // Initialise process static data
   internal::initialiseStaticData();

   // Initialise logger
   internal::cryptoLog = decaf::makeLogger("IOS_CRYPTO");

   // Initialise process heaps
   auto error = IOS_CreateCrossProcessHeap(kCrossHeapSize);
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: Failed to create cross process heap, error = {}.",
         error);
      return error;
   }

   // Setup /dev/crypto
   error = IOS_CreateMessageQueue(phys_addrof(sCryptoData->messages),
                                  sCryptoData->messages.size());
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: IOS_CreateMessageQueue failed with error = {}",
         error);
      return error;
   }
   sCryptoData->messageQueueId = static_cast<MessageQueueId>(error);

   error = MCP_RegisterResourceManager("/dev/crypto",
                                       sCryptoData->messageQueueId);
   if (error < Error::OK) {
      internal::cryptoLog->error(
         "processEntryPoint: MCP_RegisterResourceManager failed for /dev/crypto with error = {}",
         error);
      return error;
   }

   // Run /dev/crypto
   while (true) {
      error = IOS_ReceiveMessage(sCryptoData->messageQueueId, message, MessageFlags::None);
      if (error < Error::OK) {
         internal::cryptoLog->error(
            "processEntryPoint: IOS_ReceiveMessage failed with error = {}", error);
         break;
      }

      auto request = parseMessage<ResourceRequest>(message);
      switch (request->requestData.command) {
      case Command::Open:
         error = Error::NoResource;
         for (auto i = 0u; i < sCryptoData->handles.size(); ++i) {
            if (!sCryptoData->handles[i].used) {
               sCryptoData->handles[i].used = TRUE;
               sCryptoData->handles[i].processId = request->requestData.processId;
               error = static_cast<Error>(i);
               break;
            }
         }

         IOS_ResourceReply(request, error);
         break;
      case Command::Close:
         if (request->requestData.handle < static_cast<Handle>(sCryptoData->handles.size())) {
            sCryptoData->handles[request->requestData.handle].used = FALSE;
         }

         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Suspend:
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Resume:
         internal::initialiseCrypto();
         IOS_ResourceReply(request, Error::OK);
         break;
      case Command::Ioctlv:
         IOS_ResourceReply(request, internal::cryptoIoctlv(request));
         break;
      default:
         IOS_ResourceReply(request, Error::Invalid);
      }
   }
   return Error::OK;
}

} // namespace ios::crypto
