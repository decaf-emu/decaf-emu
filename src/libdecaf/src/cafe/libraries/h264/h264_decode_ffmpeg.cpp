#ifdef DECAF_FFMPEG
#include "h264.h"
#include "h264_decode.h"
#include "h264_stream.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/cafe_stackobject.h"

#include <common/align.h>
#include <common/decaf_assert.h>
#include <fmt/format.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
}

namespace cafe::h264
{

// This is decaf specific stuff - does not match structure in h264.rpl
struct H264CodecMemory
{
   AVCodecContext *context;
   AVFrame *frame;
   AVCodecParserContext *parser;
   SwsContext *sws;
   int swsWidth;
   int swsHeight;
   int inputFrameIndex;
   int outputFrameIndex;

   //! HACK: This is just a copy of the most recently seen vui_parameters in
   //! the stream, technically it should probably be the ones that are in the
   //! SPS for the given PPS in given frame's slice headers.
   uint8_t vui_parameters_present_flag;
   H264DecodedVuiParameters vui_parameters;
};

} // namespace cafe::h264

namespace cafe::h264::ffmpeg
{

static int
receiveFrames(virt_ptr<H264WorkMemory> workMemory)
{
   auto codecMemory = workMemory->codecMemory;
   auto streamMemory = workMemory->streamMemory;
   auto frame = codecMemory->frame;
   auto result = 0;

   while (result == 0) {
      result = avcodec_receive_frame(codecMemory->context, frame);
      if (result != 0) {
         break;
      }

      // Get the decoded frame info
      auto &decodedFrameInfo = streamMemory->decodedFrameInfos[codecMemory->outputFrameIndex];
      codecMemory->outputFrameIndex =
         (codecMemory->outputFrameIndex + 1) % streamMemory->decodedFrameInfos.size();


      auto decodeResult = StackObject<H264DecodeResult> { };
      decodeResult->status = 100;
      decodeResult->timestamp = decodedFrameInfo.timestamp;

      const auto pitch = align_up(frame->width, 256);

      // Destroy previously created SWS if there is different width/height
      if (codecMemory->sws &&
         (codecMemory->swsWidth != frame->width ||
          codecMemory->swsHeight != frame->height)) {
         sws_freeContext(codecMemory->sws);
         codecMemory->sws = nullptr;
      }

      // Create SWS context if needed
      if (!codecMemory->sws) {
         codecMemory->sws =
            sws_getContext(frame->width, frame->height,
                           static_cast<AVPixelFormat>(frame->format),
                           frame->width, frame->height, AV_PIX_FMT_NV12,
                           0, nullptr, nullptr, nullptr);
      }

      // Use SWS to convert frame output to NV12 format
      decaf_check(codecMemory->sws);
      auto frameBuffer = virt_cast<uint8_t *>(decodedFrameInfo.buffer);
      uint8_t *dstBuffers[] = {
         frameBuffer.get(),
         frameBuffer.get() + frame->height * pitch,
      };
      int dstStride[] = {
         pitch, pitch
      };

      sws_scale(codecMemory->sws,
                frame->data, frame->linesize,
                0, frame->height,
                dstBuffers, dstStride);

      decodeResult->framebuffer = frameBuffer;
      decodeResult->width = frame->width;
      decodeResult->height = frame->height;
      decodeResult->nextLine = pitch;

      // Copy crop
      if (frame->crop_top || frame->crop_bottom || frame->crop_left || frame->crop_right) {
         decodeResult->cropEnableFlag = uint8_t { 1 };
      } else {
         decodeResult->cropEnableFlag = uint8_t { 0 };
      }

      decodeResult->cropTop = static_cast<int32_t>(frame->crop_top);
      decodeResult->cropBottom = static_cast<int32_t>(frame->crop_bottom);
      decodeResult->cropLeft = static_cast<int32_t>(frame->crop_left);
      decodeResult->cropRight = static_cast<int32_t>(frame->crop_right);

      // Copy pan scan
      decodeResult->panScanEnableFlag = uint8_t { 0 };
      decodeResult->panScanTop = 0;
      decodeResult->panScanBottom = 0;
      decodeResult->panScanLeft = 0;
      decodeResult->panScanRight = 0;

      for (auto i = 0; i < frame->nb_side_data; ++i) {
         auto sideData = frame->side_data[i];
         if (sideData->type == AV_FRAME_DATA_PANSCAN) {
            auto panScan = reinterpret_cast<AVPanScan *>(sideData->data);

            decodeResult->panScanEnableFlag = uint8_t { 1 };
            decodeResult->panScanTop = panScan->position[0][0];
            decodeResult->panScanLeft = panScan->position[0][1];
            decodeResult->panScanRight = decodeResult->panScanLeft + panScan->width;
            decodeResult->panScanBottom = decodeResult->panScanTop + panScan->height;
         }
      }

      // Copy vui_parameters from decoded frame info
      decodeResult->vui_parameters_present_flag = decodedFrameInfo.vui_parameters_present_flag;
      if (decodeResult->vui_parameters_present_flag) {
         decodeResult->vui_parameters = virt_addrof(decodedFrameInfo.vui_parameters);
      } else {
         decodeResult->vui_parameters = nullptr;
      }

      // Invoke the frame output callback, right now this is 1 frame at a time
      // in future we may want to hoist this outside of the loop and emit N frames
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

   if (result == AVERROR_EOF || result == AVERROR(EAGAIN)) {
      // Expected return values are not an error!
      result = 0;
   } else {
      char buffer[255];
      av_strerror(result, buffer, 255);
      gLog->error("avcodec_receive_frame error: {}", buffer);
   }

   return result;
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

   auto codec = avcodec_find_decoder(AV_CODEC_ID_H264);
   if (!codec) {
      return H264Error::GenericError;
   }

   auto context = avcodec_alloc_context3(codec);
   if (!context) {
      return H264Error::GenericError;
   }

   context->flags |= AV_CODEC_FLAG_LOW_DELAY;
   context->thread_type = FF_THREAD_SLICE;
   context->pix_fmt = AV_PIX_FMT_NV12;

   if (avcodec_open2(context, codec, NULL) < 0) {
      return H264Error::GenericError;
   }

   workMemory->codecMemory->context = context;
   workMemory->codecMemory->frame = av_frame_alloc();
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

   // Open a new parser, because there is no reset function for it and I don't
   // know if it has internal state which is important :).
   workMemory->codecMemory->parser = av_parser_init(AV_CODEC_ID_H264);
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

   // Parse the bitstream looking for any SPS to grab latest vui parameters
   auto sps = StackObject<H264SequenceParameterSet> { };
   if (internal::decodeNaluSps(bitStream->buffer.get(), bitStream->buffer_length,
                               0, sps) == H264Error::OK) {
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

   // Update the decoded frame info for this frame
   auto &decodedFrameInfo = streamMemory->decodedFrameInfos[codecMemory->inputFrameIndex];
   codecMemory->inputFrameIndex =
      (codecMemory->inputFrameIndex + 1) % streamMemory->decodedFrameInfos.size();

   decodedFrameInfo.buffer = frameBuffer;
   decodedFrameInfo.timestamp = bitStream->timestamp;

   // Copy the latest VUI parameters
   // HACK: This is not technically correct and we should probably parse the
   // slice headers to see which SPS they are referencing.
   decodedFrameInfo.vui_parameters_present_flag = codecMemory->vui_parameters_present_flag;
   if (decodedFrameInfo.vui_parameters_present_flag) {
      std::memcpy(virt_addrof(decodedFrameInfo.vui_parameters).get(),
                  &codecMemory->vui_parameters,
                  sizeof(decodedFrameInfo.vui_parameters));
   }

   // Submit packet to ffmpeg
   auto packet = AVPacket { };
   av_init_packet(&packet);
   packet.data = bitStream->buffer.get();
   packet.size = bitStream->buffer_length;

   auto result = avcodec_send_packet(codecMemory->context, &packet);
   if (result != 0) {
      char buffer[255];
      av_strerror(result, buffer, 255);
      gLog->error("H264DECExecute avcodec_send_packet error: {}", buffer);
      return static_cast<H264Error>(result);
   }

   bitStream->buffer_length = 0u;

   // Read any completed frames back from ffmpeg
   result = receiveFrames(workMemory);
   if (result != 0) {
      return static_cast<H264Error>(result);
   }

   // Return 100% decoded frame
   return static_cast<H264Error>(0x80 | 100);
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

   if (workMemory->codecMemory->context) {
      // Send a null packet to flush ffmpeg decoder
      auto packet = AVPacket { };
      av_init_packet(&packet);
      packet.data = nullptr;
      packet.size = 0;
      avcodec_send_packet(workMemory->codecMemory->context, &packet);

      // Receive the flushed frames
      receiveFrames(workMemory);
   }

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

   // Flush the stream
   H264DECFlush(memory);

   // Reset the context
   if (workMemory->codecMemory->context) {
      avcodec_flush_buffers(workMemory->codecMemory->context);
   }

   if (workMemory->codecMemory->parser) {
      av_parser_close(workMemory->codecMemory->parser);
      workMemory->codecMemory->parser = nullptr;
   }

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

   av_frame_free(&workMemory->codecMemory->frame);
   avcodec_free_context(&workMemory->codecMemory->context);

   sws_freeContext(workMemory->codecMemory->sws);
   workMemory->codecMemory->sws = nullptr;

   // Just in case someone did not call H264DECEnd
   if (workMemory->codecMemory->parser) {
      av_parser_close(workMemory->codecMemory->parser);
      workMemory->codecMemory->parser = nullptr;
   }

   return H264Error::OK;
}

#if 0
// ffmpeg based H264DECCheckDecunitLength
H264Error
H264DECCheckDecunitLength(virt_ptr<void> memory,
                          virt_ptr<const uint8_t> buffer,
                          int32_t bufferLength,
                          int32_t offset,
                          virt_ptr<int32_t> outLength)
{
   auto workMemory = internal::getWorkMemory(memory);
   if (!workMemory) {
      return H264Error::InvalidParameter;
   }

   uint8_t *outBuf = nullptr;
   int outBufSize = 0;
   auto ret = av_parser_parse2(workMemory->codecMemory->parser,
                               workMemory->codecMemory->context,
                               &outBuf, &outBufSize,
                               buffer.get() + offset, bufferLength - offset,
                               AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

   if (ret < 0) {
      return H264Error::GenericError;
   }

   *outLength = outBufSize;
   return H264Error::OK;
}
#endif // if 0

} // namespace cafe::h264::ffmpeg

#endif // ifdef DECAF_FFMPEG
