#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::h264
{
constexpr auto H264SliceTypeMask = 0x1Fu;
constexpr auto H264NalSps = 7u;
constexpr auto MinimumWidth = 64u;
constexpr auto MinimumHeight = 64u;
constexpr auto MaximumWidth = 2800u;
constexpr auto MaximumHeight = 1408u;
constexpr auto BaseMemoryRequirement = 0x480000u;

void 
copy(H264DECResult& out, 
     H264DECResult  in);

int32_t 
GetNextIndex(double currentTimeStamp);

void 
TriggerCallback();

int32_t
H264DECCheckDecunitLength(virt_ptr<void> memPtr,
                          const virt_ptr<uint8_t> buf,
                          int32_t totalBytes,
                          int32_t streamOffset,
                          virt_ptr<int32_t>length);

#ifdef FFMPEG_DEC
int32_t ffmpeg_encoder_init_frame(AVFrame **framep,
                                  AVCodecContext* context,
                                  int width,
                                  int height);
#endif

} // namespace cafe::h264_dec