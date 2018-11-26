#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::h264::ffmpeg
{

H264Error
H264DECOpen(virt_ptr<void> memory);

H264Error
H264DECBegin(virt_ptr<void> memory);

H264Error
H264DECSetBitstream(virt_ptr<void> memory,
                    virt_ptr<uint8_t> buffer,
                    uint32_t bufferLength,
                    double timestamp);

H264Error
H264DECExecute(virt_ptr<void> memory,
               virt_ptr<void> frameBuffer);

H264Error
H264DECFlush(virt_ptr<void> memory);

H264Error
H264DECEnd(virt_ptr<void> memory);

H264Error
H264DECClose(virt_ptr<void> memory);

} // namespace cafe::h264::ffmpeg
