#include "cafe_loader_bounce.h"
#include "cafe_loader_entry.h"
#include "cafe_loader_error.h"
#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_iop.h"
#include "cafe_loader_purge.h"
#include "cafe_loader_setup.h"
#include "cafe_loader_query.h"
#include "cafe_loader_log.h"
#include "cafe_loader_minfileinfo.h"
#include "cafe/cafe_stackobject.h"

#include <common/align.h>
#include <zlib.h>

namespace cafe::loader::internal
{

struct SegmentBounds
{
   uint32_t min = -1;
   uint32_t max = 0;
   uint32_t allocMax = 0;
   const char *name = nullptr;
};

struct RplSegmentBounds
{
   SegmentBounds data;
   SegmentBounds load;
   SegmentBounds text;
   SegmentBounds temp;

   SegmentBounds &operator[](size_t idx)
   {
      if (idx == 0) {
         return data;
      } else if (idx == 1) {
         return load;
      } else if (idx == 2) {
         return text;
      } else {
         return temp;
      }
   }
};

static void
LiClearUserBss(bool userHasControl,
               kernel::UniqueProcessId upid,
               virt_ptr<void> base,
               uint32_t size)
{
   if (!userHasControl) {
      // Check upid
      if (upid == kernel::UniqueProcessId::HomeMenu ||
          upid == kernel::UniqueProcessId::OverlayMenu ||
          upid == kernel::UniqueProcessId::ErrorDisplay ||
          upid == kernel::UniqueProcessId::Game) {
         return;
      }
   }

   std::memset(base.get(), 0, size);
}

static int32_t
GetNextBounce(virt_ptr<LOADED_RPL> rpl)
{
   uint32_t chunkBytesRead = 0;
   auto error = LiWaitOneChunk(&chunkBytesRead,
                               virt_cast<char *>(rpl->pathBuffer).get(),
                               rpl->fileType);
   if (error != 0) {
      return error;
   }

   rpl->fileOffset = rpl->upcomingFileOffset;
   rpl->upcomingFileOffset += chunkBytesRead;
   rpl->totalBytesRead += chunkBytesRead;
   rpl->lastChunkBuffer = rpl->chunkBuffer;

   auto readBufferNumber = rpl->upcomingBufferNumber;
   if (readBufferNumber == 1) {
      rpl->upcomingBufferNumber = 2u;

      rpl->virtualFileBaseOffset += chunkBytesRead;
   } else {
      rpl->upcomingBufferNumber = 1u;

      rpl->virtualFileBase = virt_cast<void *>(virt_cast<virt_addr>(rpl->virtualFileBase) - rpl->virtualFileBaseOffset);
      rpl->virtualFileBaseOffset = chunkBytesRead;
   }

   if (chunkBytesRead == 0x400000) {
      return LiRefillUpcomingBounceBuffer(rpl, readBufferNumber);
   }

   LiInitBuffer(false);
   return 0;
}

static int32_t
sLiPrepareBounceBufferForReading(virt_ptr<LOADED_RPL> rpl,
                                 uint32_t sectionIndex,
                                 std::string_view name,
                                 uint32_t sectionOffset,
                                 uint32_t *outBytesRead,
                                 uint32_t readSize,
                                 virt_ptr<void> *outBuffer)
{
   LiCheckAndHandleInterrupts();

   if (rpl->upcomingFileOffset <= rpl->fileOffset) {
      if (sectionIndex == 0) {
         Loader_ReportError("*** {} Segment {}: bounce buffer has no size or is corrupted.",
                            rpl->moduleNameBuffer, name);
      } else {
         Loader_ReportError("*** {} Segment {}'s segment {}: bounce buffer has no size or is corrupted.",
                            rpl->moduleNameBuffer, name, sectionIndex);
      }

      LiSetFatalError(0x18729B, rpl->fileType, 1, "sLiPrepareBounceBufferForReading", 0xAD);
      return -470074;
   }

   while (sectionOffset >= rpl->upcomingFileOffset) {
      auto error = GetNextBounce(rpl);
      if (error != 0) {
         break;
      }
   }

   if (sectionOffset < rpl->fileOffset) {
      if (sectionIndex <= 0) {
         Loader_ReportError(
            "*** {} Segment {}'s segment {} offset is before the current read position.",
            rpl->moduleNameBuffer, name, sectionIndex);
      } else {
         Loader_ReportError("*** {} Segment {}'s base is before the current read position.",
                            rpl->moduleNameBuffer, name);
      }

      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLiPrepareBounceBufferForReading", 193);
      return -470075;
   }

   if (sectionOffset == rpl->upcomingFileOffset) {
      if (sectionIndex <= 0) {
         Loader_ReportError(
            "*** {} Segment {}'s segment {}: file has nothing left to read or bounce buffer is corrupted.",
            rpl->moduleNameBuffer, name, sectionIndex);
      } else {
         Loader_ReportError(
            "*** {} Segment {}: file has nothing left to read or bounce buffer is corrupted.",
            rpl->moduleNameBuffer, name);
      }
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLiPrepareBounceBufferForReading", 210);
      return -470076;
   }

   auto bytesRead = rpl->upcomingFileOffset - sectionOffset;
   if (readSize < bytesRead) {
      bytesRead = readSize;
   }

   *outBytesRead = bytesRead;
   *outBuffer = virt_cast<void *>(virt_cast<virt_addr>(rpl->virtualFileBase) + sectionOffset);
   return 0;
}

int32_t
sLiRefillBounceBufferForReading(virt_ptr<LOADED_RPL> rpl,
                                uint32_t *outBytesRead,
                                uint32_t size,
                                virt_ptr<void> *outBuffer)
{
   LiCheckAndHandleInterrupts();
   *outBytesRead = 0;
   auto result = GetNextBounce(rpl);
   if (!result) {
      auto bytesRead = rpl->upcomingFileOffset - rpl->fileOffset;
      if (size < bytesRead) {
         bytesRead = size;
      }

      *outBytesRead = bytesRead;
      *outBuffer = rpl->lastChunkBuffer;
   }

   return result;
}

int32_t
ZLIB_UncompressFromStream(virt_ptr<LOADED_RPL> rpl,
                          uint32_t sectionIndex,
                          std::string_view boundsName,
                          uint32_t fileOffset,
                          uint32_t deflatedSize,
                          virt_ptr<void> inflatedBuffer,
                          uint32_t *inflatedSize)
{
   auto inflatedBytesMax = *inflatedSize;
   auto deflatedBytesRemaining = deflatedSize;
   auto bounceBuffer = virt_ptr<void> { nullptr };
   auto bounceBufferSize = uint32_t { 0 };

   LiCheckAndHandleInterrupts();

   auto error =
      sLiPrepareBounceBufferForReading(rpl, sectionIndex, boundsName,
                                       fileOffset, &bounceBufferSize,
                                       deflatedSize, &bounceBuffer);
   if (error) {
      return error;
   }

   auto stream = z_stream { };
   std::memset(&stream, 0, sizeof(stream));

   auto zlibError = inflateInit(&stream);
   if (zlibError != Z_OK) {
      switch (zlibError) {
      case Z_STREAM_ERROR:
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 332);
         return Error::ZlibStreamError;
      case Z_MEM_ERROR:
         LiSetFatalError(0x187298u, rpl->fileType, 0, "ZLIB_UncompressFromStream", 319);
         return Error::ZlibMemError;
      case Z_VERSION_ERROR:
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 332);
         return Error::ZlibVersionError;
      default:
         Loader_ReportError("***Unknown ZLIB error {} (0x{}).", zlibError, zlibError);
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 332);
         return Error::ZlibUnknownError;
      }
   }

   constexpr auto InflateChunkSize = 0x3000u;
   rpl->lastSectionCrc = 0u;

   stream.next_out = reinterpret_cast<Bytef *>(inflatedBuffer.get());

   while (true) {
      // TODO: Loader_UpdateHeartBeat();
      LiCheckAndHandleInterrupts();
      stream.avail_in = bounceBufferSize;
      stream.next_in = reinterpret_cast<Bytef *>(bounceBuffer.get());

      while (stream.avail_in) {
         LiCheckAndHandleInterrupts();
         stream.avail_out = std::min<uInt>(InflateChunkSize, inflatedBytesMax - stream.total_out);

         zlibError = inflate(&stream, 0);
         if (zlibError != Z_OK && zlibError != Z_STREAM_END) {
            switch (zlibError) {
            case Z_STREAM_ERROR:
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 405);
               return -470086;
            case Z_MEM_ERROR:
               LiSetFatalError(0x187298u, rpl->fileType, 0, "ZLIB_UncompressFromStream", 415);
               error = -470084;
               break;
            case Z_DATA_ERROR:
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 419);
               error = -470087;
               break;
            default:
               Loader_ReportError("***Unknown ZLIB error {} (0x{}).", zlibError, zlibError);
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 424);
               error = -470100;
            }

            inflateEnd(&stream);
            return error;
         }

         decaf_check(stream.total_out <= inflatedBytesMax);
      }

      deflatedBytesRemaining -= bounceBufferSize;
      if (!deflatedBytesRemaining) {
         break;
      }

      error = sLiRefillBounceBufferForReading(rpl, &bounceBufferSize, deflatedBytesRemaining, &bounceBuffer);
      if (error) {
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "ZLIB_UncompressFromStream", 520);
         inflateEnd(&stream);
         *inflatedSize = stream.total_out;
         return -470087;
      }
   }

   inflateEnd(&stream);
   *inflatedSize = stream.total_out;
   return 0;
}

static int32_t
LiSetupOneAllocSection(kernel::UniqueProcessId upid,
                       virt_ptr<LOADED_RPL> rpl,
                       int32_t sectionIndex,
                       virt_ptr<rpl::SectionHeader> sectionHeader,
                       int32_t unk_a5,
                       SegmentBounds *bounds,
                       virt_ptr<void> base,
                       uint32_t baseAlign,
                       uint32_t unk_a9)
{
   auto globals = getGlobalStorage();
   LiCheckAndHandleInterrupts();

   auto sectionAddress = virt_cast<virt_addr>(base) + (sectionHeader->addr - bounds->min);
   rpl->sectionAddressBuffer[sectionIndex] = sectionAddress;

   if (!align_check(sectionAddress, sectionHeader->addralign)) {
      Loader_ReportError("***{} section {} alignment failure.", bounds->name, sectionIndex);
      Loader_ReportError("Ptr              = {}", sectionAddress);
      Loader_ReportError("{} base        = {}", bounds->name, base);
      Loader_ReportError("{} base align  = {}", bounds->name, baseAlign);
      Loader_ReportError("SecHdr->addr     = 0x{:08X}", sectionHeader->addr);
      Loader_ReportError("bound[{}].base = 0x{:08X}", bounds->name, bounds->min);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLiSetupOneAllocSection", 1510);
      return -470043;
   }

   if (!unk_a9 && sectionHeader->type == rpl::SHT_NOBITS && unk_a5) {
      auto userHasControl =
         (upid == kernel::UniqueProcessId::Invalid) ? true : !!globals->userHasControl;
      LiClearUserBss(userHasControl, upid, virt_cast<void *>(sectionAddress), sectionHeader->size);
   } else {
      auto bytesAvailable = uint32_t { 0 };
      auto sectionData = virt_ptr<void> { nullptr };
      auto error = sLiPrepareBounceBufferForReading(rpl,
                                                    sectionIndex,
                                                    bounds->name,
                                                    sectionHeader->offset,
                                                    &bytesAvailable,
                                                    sectionHeader->size,
                                                    &sectionData);
      if (error) {
         return error;
      }

      if (!unk_a9 && (sectionHeader->flags & rpl::SHF_DEFLATED)) {
         auto inflatedExpectedSizeBuffer = std::array<uint8_t, sizeof(uint32_t)> { };
         auto readBytes = uint32_t { 0 };

         while (readBytes < inflatedExpectedSizeBuffer.size()) {
            std::memcpy(
               inflatedExpectedSizeBuffer.data() + readBytes,
               sectionData.get(),
               std::min<size_t>(bytesAvailable, inflatedExpectedSizeBuffer.size() - readBytes));

            readBytes += bytesAvailable;
            if (readBytes >= inflatedExpectedSizeBuffer.size()) {
               break;
            }

            error =
               sLiRefillBounceBufferForReading(
                  rpl,
                  &bytesAvailable,
                  static_cast<uint32_t>(inflatedExpectedSizeBuffer.size() - readBytes),
                  &sectionData);
            if (error) {
               return error;
            }
         }

         auto inflatedExpectedSize =
            *reinterpret_cast<be2_val<uint32_t> *>(
               inflatedExpectedSizeBuffer.data());
         if (inflatedExpectedSize) {
            auto inflatedBytes = static_cast<uint32_t>(inflatedExpectedSize);
            error = ZLIB_UncompressFromStream(rpl,
                                              sectionIndex,
                                              bounds->name,
                                              sectionHeader->offset + 4,
                                              sectionHeader->size - 4,
                                              virt_cast<void *>(sectionAddress),
                                              &inflatedBytes);
            if (error) {
               Loader_ReportError(
                  "***{} {} {} Decompression ({}->{}) failure.",
                  rpl->moduleNameBuffer,
                  bounds->name,
                  sectionIndex,
                  sectionHeader->size - 4,
                  inflatedExpectedSize);
               return error;
            }

            if (inflatedBytes != inflatedExpectedSize) {
               Loader_ReportError(
                  "***{} {} {} Decompression ({}->{}) failure. Anticipated uncompressed size would be {}; got {}",
                  rpl->moduleNameBuffer,
                  bounds->name,
                  sectionIndex,
                  sectionHeader->size - 4,
                  inflatedExpectedSize,
                  inflatedExpectedSize,
                  inflatedBytes);
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLiSetupOneAllocSection", 1604);
               return -470090;
            }

            sectionHeader->size = inflatedBytes;
         }
      } else {
         auto bytesRead = 0u;
         while (true) {
            // TODO: Loader_UpdateHeartBeat();
            LiCheckAndHandleInterrupts();
            std::memcpy(virt_cast<void *>(sectionAddress + bytesRead).get(),
                        sectionData.get(),
                        bytesAvailable);
            bytesRead += bytesAvailable;

            if (bytesRead >= sectionHeader->size) {
               break;
            }

            error = sLiRefillBounceBufferForReading(rpl,
                                                    &bytesAvailable,
                                                    sectionHeader->size - bytesRead,
                                                    &sectionData);
            if (error) {
               return error;
            }
         }
      }
   }

   bounds->allocMax = std::max<uint32_t>(bounds->allocMax,
                                         sectionHeader->addr + sectionHeader->size);
   if (bounds->allocMax > bounds->max) {
      Loader_ReportError(
         "*** {} section {} segment {} makerpl's segment size was wrong: real time calculated size =0x{:08X} makerpl's size=0x{:08X}.",
         rpl->moduleNameBuffer,
         sectionIndex,
         bounds->name,
         bounds->allocMax,
         bounds->max);
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "sLiSetupOneAllocSection", 1676);
      return -470091;
   }

   return 0;
}

int32_t
LiSetupOneRPL(kernel::UniqueProcessId upid,
              virt_ptr<LOADED_RPL> rpl,
              virt_ptr<TinyHeap> codeHeapTracking,
              virt_ptr<TinyHeap> dataHeapTracking)
{
   int32_t result = 0;

   // Calculate segment bounds
   RplSegmentBounds bounds;
   bounds.data.name = "DATA";
   bounds.load.name = "LOADERINFO";
   bounds.text.name = "TEXT";
   bounds.temp.name = "TEMP";

   auto shBase = virt_cast<virt_addr>(rpl->sectionHeaderBuffer);
   for (auto i = 1u; i < rpl->elfHeader.shnum; ++i) {
      auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
      if (sectionHeader->size == 0 ||
          sectionHeader->type == rpl::SHT_RPL_FILEINFO ||
          sectionHeader->type == rpl::SHT_RPL_CRCS ||
          sectionHeader->type == rpl::SHT_RPL_IMPORTS) {
         continue;
      }

      if (sectionHeader->flags & rpl::SHF_ALLOC) {
         if ((sectionHeader->flags & rpl::SHF_EXECINSTR) &&
             sectionHeader->type != rpl::SHT_RPL_EXPORTS) {
            bounds.text.min = std::min<uint32_t>(bounds.text.min, sectionHeader->addr);
         } else {
            if (sectionHeader->flags & rpl::SHF_WRITE) {
               bounds.data.min = std::min<uint32_t>(bounds.data.min, sectionHeader->addr);
            } else {
               bounds.load.min = std::min<uint32_t>(bounds.load.min, sectionHeader->addr);
            }
         }
      } else {
         bounds.temp.min = std::min<uint32_t>(bounds.temp.min, sectionHeader->offset);
         bounds.temp.max = std::max<uint32_t>(bounds.temp.max, sectionHeader->offset + sectionHeader->size);
      }
   }

   if (bounds.data.min == -1) {
      bounds.data.min = 0;
   }

   if (bounds.load.min == -1) {
      bounds.load.min = 0;
   }

   if (bounds.text.min == -1) {
      bounds.text.min = 0;
   }

   if (bounds.temp.min == -1) {
      bounds.temp.min = 0;
   }

   auto fileInfo = rpl->fileInfoBuffer;
   bounds.text.max = (bounds.text.min + fileInfo->textSize) - fileInfo->trampAdjust;
   bounds.data.max = bounds.data.min + fileInfo->dataSize;
   bounds.load.max = (bounds.load.min + fileInfo->loadSize) - fileInfo->fileInfoPad;

   auto textSize = static_cast<uint32_t>(bounds.text.max - bounds.text.min);
   auto dataSize = static_cast<uint32_t>(bounds.data.max - bounds.data.min);
   auto loadSize = static_cast<uint32_t>(bounds.load.max - bounds.load.min);
   auto tempSize = static_cast<uint32_t>(bounds.temp.max - bounds.temp.min);

   if (fileInfo->trampAdjust >= textSize ||
       fileInfo->textSize - fileInfo->trampAdjust < textSize ||
       fileInfo->dataSize < dataSize ||
       fileInfo->loadSize - fileInfo->fileInfoPad < loadSize ||
       fileInfo->tempSize < tempSize) {
      Loader_ReportError("***Bounds check failure.");
      Loader_ReportError("b%d: %08X %08x", 0, bounds.data.min, bounds.data.max);
      Loader_ReportError("b%d: %08X %08x", 1, bounds.load.min, bounds.load.max);
      Loader_ReportError("b%d: %08X %08x", 2, bounds.text.min, bounds.text.max);
      Loader_ReportError("b%d: %08X %08x", 3, bounds.temp.min, bounds.temp.max);
      Loader_ReportError("TrampAdj = %08X", fileInfo->trampAdjust);
      Loader_ReportError("Text = %08X", fileInfo->textSize);
      Loader_ReportError("Data = %08X", fileInfo->dataSize);
      Loader_ReportError("Read = %08X", fileInfo->loadSize - fileInfo->fileInfoPad);
      Loader_ReportError("Temp = %08X", fileInfo->tempSize);
      LiSetFatalError(0x18729B, rpl->fileType, 1, "LiSetupOneRPL", 0x715);
      result = -470042;
      goto error;
   }

   if (rpl->dataBuffer) {
      for (auto i = 1u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
         LiCheckAndHandleInterrupts();

         if (sectionHeader->size &&
             !rpl->sectionAddressBuffer[i] &&
             (sectionHeader->flags & rpl::SHF_ALLOC) &&
             (sectionHeader->flags & rpl::SHF_WRITE)) {
            result = LiSetupOneAllocSection(upid,
                                            rpl,
                                            i,
                                            sectionHeader,
                                            1,
                                            &bounds.data,
                                            rpl->dataBuffer,
                                            fileInfo->dataAlign,
                                            0);
            if (result) {
               goto error;
            }
         }
      }
   }

   if (rpl->loadBuffer) {
      for (auto i = 1u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
         LiCheckAndHandleInterrupts();

         if (sectionHeader->size &&
             !rpl->sectionAddressBuffer[i] &&
             (sectionHeader->flags & rpl::SHF_ALLOC)) {
            if (sectionHeader->type == rpl::SHT_RPL_EXPORTS ||
                sectionHeader->type == rpl::SHT_RPL_IMPORTS ||
                !(sectionHeader->flags & (rpl::SHF_EXECINSTR | rpl::SHF_WRITE))) {
               result = LiSetupOneAllocSection(upid,
                                               rpl,
                                               i,
                                               sectionHeader,
                                               0,
                                               &bounds.load,
                                               rpl->loadBuffer,
                                               fileInfo->loadAlign,
                                               (sectionHeader->type == rpl::SHT_RPL_IMPORTS) ? 1 : 0);
               if (result) {
                  goto error;
               }

               if (sectionHeader->type == rpl::SHT_RPL_EXPORTS) {
                  if (sectionHeader->flags & rpl::SHF_EXECINSTR) {
                     rpl->numFuncExports = *virt_cast<uint32_t *>(rpl->sectionAddressBuffer[i]);
                     rpl->funcExports = virt_cast<void *>(rpl->sectionAddressBuffer[i] + 8);
                  } else {
                     rpl->numDataExports = *virt_cast<uint32_t *>(rpl->sectionAddressBuffer[i]);
                     rpl->dataExports = virt_cast<void *>(rpl->sectionAddressBuffer[i] + 8);
                  }
               }
            }
         }
      }
   }

   if (fileInfo->textSize) {
      if (!rpl->textBuffer) {
         Loader_ReportError("Missing TEXT allocation.");
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiSetupOneRPL", 1918);
         result = -470057;
         goto error;
      }

      for (auto i = 1u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
         LiCheckAndHandleInterrupts();

         if (sectionHeader->size &&
             !rpl->sectionAddressBuffer[i] &&
             (sectionHeader->flags & rpl::SHF_ALLOC) &&
             (sectionHeader->flags & rpl::SHF_EXECINSTR) &&
             sectionHeader->type != rpl::SHT_RPL_EXPORTS) {
            result = LiSetupOneAllocSection(upid,
                                            rpl,
                                            i,
                                            sectionHeader,
                                            0,
                                            &bounds.text,
                                            virt_cast<void *>(virt_cast<virt_addr>(rpl->textBuffer) + fileInfo->trampAdjust),
                                            fileInfo->textAlign,
                                            0);
            if (result) {
               goto error;
            }
         }
      }
   }

   if (bounds.temp.min != bounds.temp.max) {
      auto compressedRelocationsBuffer = virt_ptr<void> { nullptr };
      auto compressedRelocationsBufferSize = uint32_t { 0 };
      auto memoryAvailable = uint32_t { 0 };
      auto tempSize = bounds.temp.max - bounds.temp.min;
      auto dataSize = uint32_t { 0 };
      auto data = virt_ptr<void> { nullptr };
      auto readBytes = 0u;

      result = LiCacheLineCorrectAllocEx(codeHeapTracking,
                                         tempSize,
                                         -32,
                                         &compressedRelocationsBuffer,
                                         1,
                                         &compressedRelocationsBufferSize,
                                         &memoryAvailable,
                                         rpl->fileType);
      if (result) {
         Loader_ReportError(
            "*** allocation failed for {} size = {}, align = {} from {} heap;  (needed {}, available {}).",
            "compressed relocations",
            tempSize,
            -32,
            "RPL Code",
            compressedRelocationsBufferSize,
            memoryAvailable);
         goto error;
      }

      rpl->compressedRelocationsBuffer = compressedRelocationsBuffer;
      rpl->compressedRelocationsBufferSize = compressedRelocationsBufferSize;

      result = sLiPrepareBounceBufferForReading(rpl, 0, bounds.temp.name, bounds.temp.min, &dataSize, tempSize, &data);
      if (result) {
         goto error;
      }

      while (tempSize > 0) {
         // TODO: Loader_UpdateHeartBeat
         LiCheckAndHandleInterrupts();
         std::memcpy(virt_cast<void *>(virt_cast<virt_addr>(compressedRelocationsBuffer) + readBytes).get(),
                     data.get(),
                     dataSize);
         readBytes += dataSize;
         tempSize -= dataSize;

         if (!tempSize) {
            break;
         }

         result = sLiRefillBounceBufferForReading(rpl, &dataSize, tempSize, &data);
         if (result) {
            goto error;
         }
      }

      for (auto i = 1u; i < rpl->elfHeader.shnum; ++i) {
         auto sectionHeader = virt_cast<rpl::SectionHeader *>(shBase + i * rpl->elfHeader.shentsize);
         LiCheckAndHandleInterrupts();

         if (sectionHeader->size && !rpl->sectionAddressBuffer[i]) {
            rpl->sectionAddressBuffer[i] = virt_cast<virt_addr>(compressedRelocationsBuffer) + (sectionHeader->offset - bounds.temp.min);
            bounds.temp.allocMax = std::max<uint32_t>(bounds.temp.allocMax,
                                                      sectionHeader->addr + sectionHeader->size);

            if (bounds.temp.allocMax > bounds.temp.max) {
               Loader_ReportError(
                  "***Section {} segment {} makerpl's section size was wrong: mRealTimeLimit=0x%08X, mLimit=0x%08X. Error is Loader's fault.",
                  i,
                  bounds.temp.name,
                  bounds.temp.allocMax,
                  bounds.temp.max);
               LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiSetupOneRPL", 2034);
               result = -470091;
               goto error;
            }
         }
      }
   }

   for (auto i = 0u; i < 4; ++i) {
      if (bounds[i].allocMax > bounds[i].max) {
         Loader_ReportError(
            "***Segment %s makerpl's segment size was wrong: mRealTimeLimit=0x%08X, mLimit=0x%08X. Error is Loader's fault.\n",
            bounds[i].name,
            bounds[i].allocMax,
            bounds[i].max);
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LiSetupOneRPL", 2052);
         result = -470091;
         goto error;
      }
   }

   rpl->postTrampBuffer = align_up(virt_cast<virt_addr>(rpl->textBuffer) + fileInfo->trampAdjust + (bounds.text.allocMax - bounds.text.min), 16);

   rpl->textAddr = virt_cast<virt_addr>(rpl->textBuffer) + fileInfo->trampAdjust;
   rpl->textOffset = rpl->textAddr - bounds.text.min;
   rpl->textSize = static_cast<uint32_t>(bounds.text.max - bounds.text.min);

   rpl->dataAddr = virt_cast<virt_addr>(rpl->dataBuffer);
   rpl->dataOffset = rpl->dataAddr - bounds.data.min;
   rpl->dataSize = static_cast<uint32_t>(bounds.data.max - bounds.data.min);

   rpl->loadAddr = virt_cast<virt_addr>(rpl->loadBuffer);
   rpl->loadOffset = rpl->loadAddr - bounds.load.min;
   rpl->loadSize = static_cast<uint32_t>(bounds.load.max - bounds.load.min);

   rpl->loadStateFlags |= LoadStateFlags::LoaderSetup;

   result = LiCleanUpBufferAfterModuleLoaded();
   if (result) {
      goto error;
   }

   return 0;

error:
   if (rpl->compressedRelocationsBuffer) {
      LiCacheLineCorrectFreeEx(codeHeapTracking,
                                 rpl->compressedRelocationsBuffer,
                                 rpl->compressedRelocationsBufferSize);
      rpl->compressedRelocationsBuffer = nullptr;
   }

   LiCloseBufferIfError();
   Loader_ReportError("***LiSetupOneRPL({}) failed with err={}.", rpl->moduleNameBuffer, result);
   return result;
}

static bool
sCheckDataRange(virt_addr address,
                uint32_t maxDataSize)
{

   return address >= virt_addr { 0x10000000 } &&
          address < (virt_addr { 0x10000000 } + maxDataSize);
}

static int32_t
sValidateSetupParams(virt_addr address,
                     uint32_t size,
                     uint32_t align,
                     uint32_t maxSize,
                     int32_t error,
                     const char *areaName)
{
   if (!sCheckDataRange(address, maxSize)) {
      Loader_ReportError(
         "*** invalid {} area address. apArea={}, lo addr={}, hi addr={}",
         areaName,
         address,
         virt_addr { 0x10000000 },
         virt_addr { maxSize + 0x10000000 });
      return error;
   }

   if (!sCheckDataRange(address + size, maxSize)) {
      Loader_ReportError(
         "*** invalid {} area buffer. apArea+aAreaBytes={}, lo addr={}, hi addr={}",
         areaName,
         address + size,
         virt_addr { 0x10000000 },
         virt_addr { maxSize + 0x10000000 });
      return error;
   }

   if (!align_check(address, align)) {
      Loader_ReportError("*** invalid {} area buffer alignment.", areaName);
      return error;
   }

   if (size && !Loader_ValidateAddrRange(address, size)) {
      Loader_ReportError("*** invalid {} area buffer range {}..{}.",
                         areaName, address, address + size);
      return error;
   }

   return 0;
}

int32_t
LOADER_Setup(kernel::UniqueProcessId upid,
             LOADER_Handle handle,
             BOOL isPurge,
             virt_ptr<LOADER_MinFileInfo> minFileInfo)
{
   auto globals = getGlobalStorage();
   if (globals->currentUpid != upid) {
      Loader_ReportError("*** Loader address space not set for process {} but called for {}.",
                         static_cast<int>(globals->currentUpid.value()),
                         static_cast<int>(upid));
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Setup", 2184);
      LiCloseBufferIfError();
      return Error::DifferentProcess;
   }

   if (minFileInfo) {
      if (auto error = LiValidateMinFileInfo(minFileInfo, "LOADER_Setup")) {
         LiCloseBufferIfError();
         return error;
      }
   }

   auto error = LiValidateAddress(handle, 0, 0, -470046,
                                  virt_addr { 0x02000000 },
                                  virt_addr { 0x10000000 },
                                  "kernel handle for module (out of valid range-read)");
   if (error) {
      if (getProcFlags().disableSharedLibraries()) {
         Loader_ReportError("*** bad kernel handle for module (out of valid range-read)");
         LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Setup", 2215);
         LiCloseBufferIfError();
         return error;
      }

      error = LiValidateAddress(handle, 0, 0, -470046,
                                virt_addr { 0xEFE0B000 },
                                virt_addr { 0xEFE80000 },
                                "kernel handle for module (out of valid range-read)");
      if (error) {
         LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Setup", 2215);
         LiCloseBufferIfError();
         return error;
      }
   }

   // Find module
   auto prev = virt_ptr<LOADED_RPL> { nullptr };
   auto rpl = virt_ptr<LOADED_RPL> { nullptr };
   for (rpl = globals->firstLoadedRpl; rpl; rpl = rpl->nextLoadedRpl) {
      if (rpl->moduleNameBuffer == handle) {
         break;
      }

      prev = rpl;
   }

   if (!rpl) {
      LiSetFatalError(0x18729Bu, 0, 1, "LOADER_Setup", 2240);
      LiCloseBufferIfError();
      return -470010;
   }

   // If this is a purge command, perform the purge
   if (isPurge) {
      if (rpl->loadStateFlags & LoaderStateFlag4) {
         return 0;
      }

      // Remove from linked list
      if (prev) {
         prev->nextLoadedRpl = rpl->nextLoadedRpl;
      }

      if (!rpl->nextLoadedRpl) {
         globals->lastLoadedRpl = prev;
      }

      LiPurgeOneUnlinkedModule(rpl);
      return 0;
   }

   // Check if we have already setup
   if (rpl->loadStateFlags & LoaderSetup) {
      Loader_ReportError("*** module already set up.");
      LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_Setup", 2267);
      LiCloseBufferIfError();
      return -470047;
   }

   // Check input
   if (minFileInfo->dataSize) {
      if (auto error = sValidateSetupParams(virt_cast<virt_addr>(minFileInfo->dataBuffer),
                                            minFileInfo->dataSize,
                                            minFileInfo->dataAlign,
                                            globals->maxDataSize,
                                            -470021,
                                            "data")) {
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_Setup", 2283);
         LiCloseBufferIfError();
         return error;
      }
   }

   if (minFileInfo->loadSize) {
      if (auto error = sValidateSetupParams(virt_cast<virt_addr>(minFileInfo->loadBuffer),
                                            minFileInfo->loadSize,
                                            minFileInfo->loadAlign,
                                            globals->maxDataSize,
                                            -470048,
                                            "loader_info")) {
         LiSetFatalError(0x18729Bu, rpl->fileType, 1, "LOADER_Setup", 2300);
         LiCloseBufferIfError();
         return error;
      }
   }

   // Perform setup
   rpl->dataBuffer = minFileInfo->dataBuffer;
   rpl->loadBuffer = minFileInfo->loadBuffer;
   if (auto error = LiSetupOneRPL(upid,
                                  rpl,
                                  globals->processCodeHeap,
                                  globals->processCodeHeap)) {
      rpl->dataBuffer = nullptr;
      rpl->loadBuffer = nullptr;
      LiCloseBufferIfError();
      return error;
   }

   // Do queries
   if (auto error = LOADER_GetSecInfo(upid, handle,
                                      minFileInfo->outNumberOfSections,
                                      minFileInfo->outSectionInfo)) {
      LiCloseBufferIfError();
      return error;
   }

   if (auto error = LOADER_GetFileInfo(upid, handle,
                                       minFileInfo->outSizeOfFileInfo,
                                       minFileInfo->outFileInfo,
                                       nullptr, nullptr)) {
      LiCloseBufferIfError();
      return error;
   }

   if (auto error = LOADER_GetPathString(upid, handle,
                                         minFileInfo->outPathStringSize,
                                         minFileInfo->pathStringBuffer,
                                         minFileInfo->outFileInfo)) {
      LiCloseBufferIfError();
      return error;
   }

   return 0;
}

} // namespace cafe::loader::internal
