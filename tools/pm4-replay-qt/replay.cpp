#include "replay.h"
#include <libgpu/latte/latte_enum_as_string.h>

std::shared_ptr<ReplayFile>
openReplay(const std::string &path)
{
   auto fileSize = size_t { 0 };
   auto fileHandle = platform::openMemoryMappedFile(path, platform::ProtectFlags::ReadOnly, &fileSize);
   if (fileHandle == platform::InvalidMapFileHandle) {
      return nullptr;
   }

   // Map filew
   auto fileView = platform::mapViewOfFile(fileHandle, platform::ProtectFlags::ReadOnly, 0, fileSize);
   if (!fileView) {
      platform::closeMemoryMappedFile(fileHandle);
      return nullptr;
   }

   // Sanity check the magic header
   auto magic = *reinterpret_cast<std::array<char, 4> *>(fileView);
   if (magic != decaf::pm4::CaptureMagic) {
      platform::unmapViewOfFile(fileView, fileSize);
      platform::closeMemoryMappedFile(fileHandle);
      return nullptr;
   }

   auto replay = std::make_shared<ReplayFile>();
   replay->handle = fileHandle;
   replay->view = reinterpret_cast<uint8_t *>(fileView);
   replay->size = fileSize;
   return replay;
}

static bool
buildIndexCommandBuffer(std::shared_ptr<ReplayFile> replay,
                        size_t filePos,
                        size_t numWords)
{
   auto buffer = reinterpret_cast<be2_val<uint32_t> *>(replay->view + filePos);

   for (auto pos = size_t { 0u }; pos < numWords; ) {
      auto header = Header::get(buffer[pos]);
      auto size = size_t { 0u };

      switch (header.type()) {
      case PacketType::Type0:
      {
         auto header0 = HeaderType0::get(header.value);
         size = header0.count() + 1;
         break;
      }
      case PacketType::Type3:
      {
         auto header3 = HeaderType3::get(header.value);
         size = header3.size() + 1;

         if (header3.opcode() == IT_OPCODE::DECAF_SWAP_BUFFERS) {
            replay->index.frames.push_back({ ReplayPosition { replay->index.packets.size(), replay->index.commands.size() } });
         }

         break;
      }
      case PacketType::Type2:
      {
         // This is a filler packet, like a "nop", ignore it
         break;
      }
      case PacketType::Type1:
      default:
         size = numWords;
         break;
      }

      replay->index.commands.push_back({ header, buffer + pos });
      pos += size + 1;
   }

   return true;
}

bool
buildReplayIndex(std::shared_ptr<ReplayFile> replay)
{
   size_t pos = 4;
   replay->index.frames.push_back({ ReplayPosition { 0, 0 } });

   while (pos + sizeof(decaf::pm4::CapturePacket) <= replay->size) {
      auto packet = reinterpret_cast<decaf::pm4::CapturePacket *>(replay->view + pos);
      pos += sizeof(decaf::pm4::CapturePacket);

      if (pos + packet->size > replay->size) {
         break;
      }

      switch (packet->type) {
      case decaf::pm4::CapturePacket::CommandBuffer:
      {
         buildIndexCommandBuffer(replay, pos, packet->size / 4);
         break;
      }
      case decaf::pm4::CapturePacket::RegisterSnapshot:
      {
         break;
      }
      case decaf::pm4::CapturePacket::SetBuffer:
      {
         break;
      }
      case decaf::pm4::CapturePacket::MemoryLoad:
      {
         break;
      }
      default:
         break;
      }

      replay->index.packets.push_back({ packet->type, packet->size, replay->view + pos });
      pos += packet->size;
   }

   return true;
}

std::string
getCommandName(ReplayIndex::Command &command)
{
   switch (command.header.type()) {
   case PacketType::Type0:
      return "PM4::Type0 Unknown";
   case PacketType::Type1:
      return "PM4::Type1 Unknown";
   case PacketType::Type2:
      return "PM4::Type2 Unknown";
   case PacketType::Type3:
   {
      auto header3 = HeaderType3::get(command.header.value);
      return latte::pm4::to_string(header3.opcode());
   }
   default:
      return "Unknown";
   }
}
