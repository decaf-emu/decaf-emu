#include "h264.h"
#include "h264_decode.h"
#include "h264_stream.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/cafe_stackobject.h"

#include <fmt/format.h>

namespace cafe::h264
{

// This is decaf specific stuff - does not match structure in h264.rpl
struct H264CodecMemory
{
   int inputFrameIndex;
   int outputFrameIndex;
   int width;
   int height;

   //! HACK: This is just a copy of the most recently seen vui_parameters in
   //! the stream, technically it should probably be the ones that are in the
   //! SPS for the given PPS in given frame's slice headers.
   uint8_t vui_parameters_present_flag;
   H264DecodedVuiParameters vui_parameters;
};

} // namespace cafe::h264

namespace cafe::h264::null
{

static constexpr uint8_t NullImage[6][20] = {
   // H264
   { 1,0,0,1, 0, 1,1,1,0, 0, 0,1,1,0, 0, 1,0,0,1, 0, },
   { 1,0,0,1, 0, 0,0,0,1, 0, 1,0,0,0, 0, 1,0,0,1, 0, },
   { 1,1,1,1, 0, 0,1,1,0, 0, 1,1,1,0, 0, 1,1,1,1, 0, },
   { 1,0,0,1, 0, 1,0,0,0, 0, 1,0,0,1, 0, 0,0,0,1, 0, },
   { 1,0,0,1, 0, 1,1,1,1, 0, 0,1,1,0, 0, 0,0,0,1, 0, },
   { 0,0,0,0, 0, 0,0,0,0, 0, 0,0,0,0, 0, 0,0,0,0, 0, },
};

static int
receiveFrames(virt_ptr<H264WorkMemory> workMemory)
{
   auto codecMemory = workMemory->codecMemory;
   auto streamMemory = workMemory->streamMemory;
   const auto pitch = align_up(codecMemory->width, 256);

   while (codecMemory->outputFrameIndex != codecMemory->inputFrameIndex) {
      // Get the decoded frame info
      auto &decodedFrameInfo = streamMemory->decodedFrameInfos[codecMemory->outputFrameIndex];
      codecMemory->outputFrameIndex =
         (codecMemory->outputFrameIndex + 1) % streamMemory->decodedFrameInfos.size();

      // Fill framebuffer with our fake image
      auto frameBuffer = virt_cast<uint8_t *>(decodedFrameInfo.buffer);
      auto dst = frameBuffer.get();

      // Y
      for (auto y = 0; y < codecMemory->height; ++y) {
         for (auto x = 0; x < codecMemory->width; ++x) {
            dst[x] = NullImage[(y / 2) % 6][(x / 2) % 20] ? 0xFF : 0x00;
         }

         dst += pitch;
      }

      // Interleaved UV
      for (auto y = 0; y < codecMemory->height / 2; ++y) {
         for (auto x = 0; x < codecMemory->width / 2; ++x) {
            dst[x * 2 + 0] = x % 255;
            dst[x * 2 + 1] = y % 255;
         }

         dst += pitch;
      }

      auto decodeResult = StackObject<H264DecodeResult> { };
      decodeResult->status = 100;
      decodeResult->timestamp = decodedFrameInfo.timestamp;
      decodeResult->framebuffer = frameBuffer;
      decodeResult->width = codecMemory->width;
      decodeResult->height = codecMemory->height;
      decodeResult->nextLine = pitch;

      // Crop
      decodeResult->cropEnableFlag = uint8_t { 0 };
      decodeResult->cropTop = 0;
      decodeResult->cropBottom = 0;
      decodeResult->cropLeft = 0;
      decodeResult->cropRight = 0;

      // Copy pan scan
      decodeResult->panScanEnableFlag = uint8_t { 0 };
      decodeResult->panScanTop = 0;
      decodeResult->panScanBottom = 0;
      decodeResult->panScanLeft = 0;
      decodeResult->panScanRight = 0;

      // Copy vui_parameters from decoded frame info
      decodeResult->vui_parameters_present_flag = decodedFrameInfo.vui_parameters_present_flag;
      if (decodeResult->vui_parameters_present_flag) {
         decodeResult->vui_parameters = virt_addrof(decodedFrameInfo.vui_parameters);
      } else {
         decodeResult->vui_parameters = nullptr;
      }

      // Invoke the frame output callback
      auto results = StackArray<virt_ptr<H264DecodeResult>, 5> { };
      auto output = StackObject<H264DecodeOutput> { };
      output->frameCount = 1;
      output->decodeResults = results;
      output->userMemory = streamMemory->paramUserMemory;
      results[0] = decodeResult;

      cafe::invoke(cpu::this_core::state(),
                   streamMemory->paramFramePointerOutput,
                   output);
   }

   return H264Error::OK;
}


/**
 * Open a H264 decoder.
 */
H264Error
H264DECOpen(virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   return H264Error::OK;
}


/**
 * Prepare for decoding.
 */
H264Error
H264DECBegin(virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   std::memset(virt_addrof(workMemory->streamMemory->decodedFrameInfos).get(),
               0,
               sizeof(workMemory->streamMemory->decodedFrameInfos));
   workMemory->codecMemory->inputFrameIndex = 0;
   workMemory->codecMemory->outputFrameIndex = 0;
   return H264Error::OK;
}


/**
 * Set the bit stream to be read for decoding.
 */
H264Error
H264DECSetBitstream(virt_ptr<void> memory,
                    virt_ptr<uint8_t> buffer,
                    uint32_t bufferLength,
                    double timestamp)
{
   if (!memory || !buffer || bufferLength < 4) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   workMemory->bitStream->buffer = buffer;
   workMemory->bitStream->buffer_length = bufferLength;
   workMemory->bitStream->bit_position = 0u;
   workMemory->bitStream->timestamp = timestamp;
   return H264Error::OK;
}


/**
 * Perform decoding of the bitstream and put the output frame into frameBuffer.
 */
H264Error
H264DECExecute(virt_ptr<void> memory,
               virt_ptr<void> frameBuffer)
{
   if (!memory || !frameBuffer) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   auto bitStream = workMemory->bitStream;
   auto codecMemory = workMemory->codecMemory;
   auto streamMemory = workMemory->streamMemory;

   if (!bitStream->buffer_length) {
      return H264Error::GenericError;
   }

   // Parse the bitstream looking for any SPS
   auto sps = StackObject<H264SequenceParameterSet> { };
   if (internal::decodeNaluSps(bitStream->buffer.get(),
                               bitStream->buffer_length,
                               0, sps) == H264Error::OK) {
      codecMemory->width = sps->pic_width;
      codecMemory->height = sps->pic_height;

      if (!sps->frame_mbs_only_flag) {
         codecMemory->height = 2 * sps->pic_height;
      }

      // Copy VUI parameters from the SPS
      codecMemory->vui_parameters_present_flag = sps->vui_parameters_present_flag;

      if (sps->vui_parameters_present_flag) {
         auto &vui = codecMemory->vui_parameters;
         vui.aspect_ratio_info_present_flag = sps->vui_aspect_ratio_info_present_flag;
         vui.aspect_ratio_idc = sps->vui_aspect_ratio_idc;
         vui.sar_width = sps->vui_sar_width;
         vui.sar_height = sps->vui_sar_height;
         vui.overscan_info_present_flag = sps->vui_overscan_info_present_flag;
         vui.overscan_appropriate_flag = sps->vui_overscan_appropriate_flag;
         vui.video_signal_type_present_flag = sps->vui_video_signal_type_present_flag;
         vui.video_format = sps->vui_video_format;
         vui.video_full_range_flag = sps->vui_video_full_range_flag;
         vui.colour_description_present_flag = sps->vui_colour_description_present_flag;
         vui.colour_primaries = sps->vui_colour_primaries;
         vui.transfer_characteristics = sps->vui_transfer_characteristics;
         vui.matrix_coefficients = sps->vui_matrix_coefficients;
         vui.chroma_loc_info_present_flag = sps->vui_chroma_loc_info_present_flag;
         vui.chroma_sample_loc_type_top_field = sps->vui_chroma_sample_loc_type_top_field;
         vui.chroma_sample_loc_type_bottom_field = sps->vui_chroma_sample_loc_type_bottom_field;
         vui.timing_info_present_flag = sps->vui_timing_info_present_flag;
         vui.num_units_in_tick = sps->vui_num_units_in_tick;
         vui.time_scale = sps->vui_time_scale;
         vui.fixed_frame_rate_flag = sps->vui_fixed_frame_rate_flag;
         vui.nal_hrd_parameters_present_flag = sps->vui_nal_hrd_parameters_present_flag;
         vui.vcl_hrd_parameters_present_flag = sps->vui_vcl_hrd_parameters_present_flag;
         vui.low_delay_hrd_flag = sps->vui_low_delay_hrd_flag;
         vui.pic_struct_present_flag = sps->vui_pic_struct_present_flag;
         vui.bitstream_restriction_flag = sps->vui_bitstream_restriction_flag;
         vui.motion_vectors_over_pic_boundaries_flag = sps->vui_motion_vectors_over_pic_boundaries_flag;
         vui.max_bytes_per_pic_denom = sps->vui_max_bytes_per_pic_denom;
         vui.max_bits_per_mb_denom = sps->vui_max_bits_per_mb_denom;
         vui.log2_max_mv_length_horizontal = sps->vui_log2_max_mv_length_horizontal;
         vui.log2_max_mv_length_vertical = sps->vui_log2_max_mv_length_vertical;
         vui.num_reorder_frames = sps->vui_num_reorder_frames;
         vui.max_dec_frame_buffering = sps->vui_max_dec_frame_buffering;
      }
   }
   bitStream->buffer_length = 0u;

   // Update the decoded frame info for this frame
   auto &decodedFrameInfo = streamMemory->decodedFrameInfos[codecMemory->inputFrameIndex];
   codecMemory->inputFrameIndex =
      (codecMemory->inputFrameIndex + 1) % streamMemory->decodedFrameInfos.size();

   decodedFrameInfo.buffer = frameBuffer;
   decodedFrameInfo.timestamp = bitStream->timestamp;
   decodedFrameInfo.vui_parameters_present_flag = codecMemory->vui_parameters_present_flag;
   if (decodedFrameInfo.vui_parameters_present_flag) {
      std::memcpy(virt_addrof(decodedFrameInfo.vui_parameters).get(),
                  &codecMemory->vui_parameters,
                  sizeof(decodedFrameInfo.vui_parameters));
   }


   auto result = receiveFrames(workMemory);
   if (result == 0) {
      return static_cast<H264Error>(0x80 | 100);
   }

   return static_cast<H264Error>(result);
}


/**
 * Flush any internally buffered frames.
 */
H264Error
H264DECFlush(virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   // We do not buffer frames, so nothing to flush.
   return H264Error::OK;
}


/**
 * End decoding of the current stream.
 */
H264Error
H264DECEnd(virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   H264DECFlush(memory);
   return H264Error::OK;
}


/**
 * Cleanup the decoder.
 */
H264Error
H264DECClose(virt_ptr<void> memory)
{
   if (!memory) {
      return H264Error::InvalidParameter;
   }

   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   return H264Error::OK;
}

} // namespace cafe::h264::null
