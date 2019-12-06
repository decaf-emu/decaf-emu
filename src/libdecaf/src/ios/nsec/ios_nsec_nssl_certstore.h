#pragma once
#include "ios_nsec_enum.h"
#include "ios_nsec_nssl_types.h"

#include "ios/ios_enum.h"
#include "ios/ios_error.h"
#include "ios/ios_ipc.h"

#include <cstdint>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>
#include <optional>

namespace ios::nsec::internal
{

#pragma pack(push, 1)

// Reversing IOS seems to indicate this may be 1000, but lets just set to 100
// to save memory.
constexpr auto MaxNumCertificates = 100u;

struct CertStoreMetaData
{
   be2_val<NSSLCertID> id;
   be2_val<int32_t> type; // This is NOT an NSSLCertType
   be2_val<NSSLCertEncoding> encoding;
   be2_val<NSSLCertProperties> properties;
   be2_val<int32_t> groups;
   be2_val<uint64_t> capabilityMask;
   be2_array<ProcessId, 33> processIds;
   be2_array<TitleId, 33> titleIds;
   be2_val<int32_t> numPaths;
   be2_array<char, 128> path1;
   be2_array<char, 128> path2;
   be2_val<int32_t> rawE0Size;
   be2_val<int32_t> rawE1Size;
};
CHECK_OFFSET(CertStoreMetaData, 0x00, id);
CHECK_OFFSET(CertStoreMetaData, 0x04, type);
CHECK_OFFSET(CertStoreMetaData, 0x08, encoding);
CHECK_OFFSET(CertStoreMetaData, 0x0C, properties);
CHECK_OFFSET(CertStoreMetaData, 0x10, groups);
CHECK_OFFSET(CertStoreMetaData, 0x14, capabilityMask);
CHECK_OFFSET(CertStoreMetaData, 0x1C, processIds);
CHECK_OFFSET(CertStoreMetaData, 0xA0, titleIds);
CHECK_OFFSET(CertStoreMetaData, 0x1A8, numPaths);
CHECK_OFFSET(CertStoreMetaData, 0x1AC, path1);
CHECK_OFFSET(CertStoreMetaData, 0x22C, path2);
CHECK_OFFSET(CertStoreMetaData, 0x2AC, rawE0Size);
CHECK_OFFSET(CertStoreMetaData, 0x2B0, rawE1Size);
CHECK_SIZE(CertStoreMetaData, 0x2B4);

#pragma pack(pop)

bool
checkCertPermission(phys_ptr<CertStoreMetaData> certMetaData,
                    TitleId titleId, ProcessId processId, uint64_t caps);

bool
checkCertExportable(phys_ptr<CertStoreMetaData> certMetaData);

std::optional<uint32_t>
getCertFileSize(phys_ptr<CertStoreMetaData> certMetaData,
                int32_t pathIndex);


std::optional<uint32_t>
getCertFileData(phys_ptr<CertStoreMetaData> certMetaData,
                int32_t pathIndex,
                phys_ptr<uint8_t> certBuffer,
                uint32_t certBufferSize);

phys_ptr<CertStoreMetaData>
lookupCertMetaData(NSSLCertID id);

Error
loadCertstoreMetadata();

void
initialiseStaticCertStoreData();

} // namespace ios::nsec::internal
