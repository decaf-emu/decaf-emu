#include "h264.h"
#include "h264_dec.h"
#include "h264_stream.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_interrupts.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_systeminfo.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "cafe/libraries/coreinit/coreinit_time.h"

#include <array>
#include <common/align.h> 
#include <common/decaf_assert.h>
#include <fmt/format.h>

#include <fstream>


using namespace cafe::coreinit;

namespace cafe::h264
{

h264_stream_t* h = h264_new();

virt_ptr< H264DECOutput> outp;

int32_t
H264DECInitParam(int32_t memSize,
                 virt_ptr<void> memPtr)
{
   return 0;
}

int32_t
H264DECSetParam(virt_ptr<void> memPtr,
                int32_t paramid,
                virt_ptr<void> param)
{
   switch (paramid)
   {
   case 0x00000001:
      sH264Data->callback = virt_func_cast<H264Callback>(virt_cast<virt_addr>(param));
      break;
   case 0x20000002:
      {
         uint8_t b = *((uint8_t*)param.getRawPointer());
         sH264Data->buffered = b ? false : true;
      }
      break;
   case 0x70000001:
      outp->UserMemory = param;
      break;
   case 0x02300da0:
      break;
   default:
      break;
   }
   return 0;
}

int32_t
H264DECOpen(virt_ptr<void> memPtr)
{
#ifdef FFMPEG_DEC
   currentFrame = 0;

   AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
   if (!codec) {
      return -1;
   }

   context = avcodec_alloc_context3(codec);
   if (!context) {
      return -1;
   }

   context->bit_rate = 400000;
   context->time_base.num = 1;
   context->time_base.den = 25;
   context->gop_size = 10;
   context->max_b_frames = MAX_FRAMES;
   context->pix_fmt = AV_PIX_FMT_YUV420P;

   if (avcodec_open2(context, codec, NULL) < 0) {
      return -1;
   }

   frame = av_frame_alloc();
   if (!frame) {
      return -1;
   }

   frame->width = width;
   frame->height = height;

   sws = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV420P, frame->width, frame->height, AV_PIX_FMT_NV12, 2, NULL, NULL, NULL);
   if (ffmpeg_encoder_init_frame(&frame2, context, frame->width, frame->height)) {
      gLog->error("H264DECOpen({}) -> Frame failed to decoded successfully (ffmpeg_encoder_init_frame)", memPtr);
      return -1;
   }
#endif

   return 0;
}



int32_t
H264DECBegin(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECSetBitstream(virt_ptr<void> memPtr,
                    const virt_ptr<uint8_t> bitstream,
                    const int32_t length,
                    const double timeStamp)
{
   int result = 0;

#ifdef FFMPEG_DEC
   avpkt.size = length;
   if (avpkt.size == 0) {
      return -1;
   }

   frame->format = context->pix_fmt;

   avpkt.data = (uint8_t*)std::malloc(length);

   for (int i = 0; i < length; ++i) {
      avpkt.data[i] = bitstream[i];
   }

   int frame_count;
   result = avcodec_decode_video2(context, frame, &frame_count, &avpkt);

   frame->format = context->pix_fmt;
   frame->width = width;
   frame->height = height;

   outp->FmCnt = 1;
   sH264Data->outputResults[currentFrame].TimeStamp = timeStamp;
   sH264Data->outputResults[currentFrame].DecStatus = 100;
   if (result < 0) {
      gLog->error("H264DECSetBitstream -> Error decoding stream");
   } else {
      avpkt.size = result;
      result = 0;
   }
#else 
   sH264Data->outputResults[currentFrame].TimeStamp = timeStamp;
   sH264Data->outputResults[currentFrame].DecStatus = 100;
#endif

   return result;
}

int32_t
H264DECExecute(virt_ptr<void> memPtr,
               virt_ptr<void> StrFmPtr)
{
#ifdef FFMPEG_DEC
   int num_bytes = avpicture_get_size((AVPixelFormat)(frame->format), frame->width, frame->height);
   int num_bytes2;

   if (num_bytes > 0) {
      uint8_t* frame2_buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
      int ret = avpicture_fill((AVPicture*)frame2, frame2_buffer, AV_PIX_FMT_NV12, frame->width, frame->height);

      int height = sws_scale(sws, frame->data, frame->linesize, 0, frame->height, frame2->data, frame2->linesize);

      num_bytes2 = avpicture_get_size(AV_PIX_FMT_NV12, frame2->width, frame2->height);
      
      if (num_bytes2 > 0) {
         memcpy(StrFmPtr.getRawPointer(), frame2->data[0], num_bytes2);
         gLog->debug("H264DECExecute({}, {}) -> Frame decoded successfully (W={},H={},F={})", memPtr, StrFmPtr, frame->width, frame->height, frame->format);
         sH264Data->outputResults[currentFrame].Result = StrFmPtr;
         currentFrame++;       
      } else {
         gLog->error("H264DECExecute({}, {}) -> Frame failed to decoded successfully (num_bytes2 W={},H={},F={})", memPtr, StrFmPtr, frame->width, frame->height, frame->format);
      }
   } else {
      gLog->error("H264DECExecute({}, {}) -> Frame failed to decoded successfully (num_bytes W={},H={},F={})", memPtr, StrFmPtr, frame->width, frame->height, frame->format);
   }

#else
   int32_t imageSize = (width * height) + (width * (height / 2));

   if (imageSize > 0) {

      int p = 0;

      uint8_t* d = (uint8_t*)malloc(imageSize);

      uint8_t R = 128;
      uint8_t G = 32;
      uint8_t B = 192;

      uint8_t Y = static_cast<uint8_t>((0.257 * R) + (0.504 * G) + (0.098 * B) + 16);
      uint8_t V = static_cast<uint8_t>((0.439 * R) - (0.368 * G) - (0.071 * B) + 128);
      uint8_t U = -static_cast<uint8_t>((0.148 * R) - (0.291 * G) + (0.439 * B) + 128);

      for (int h = 0; h < height; h++) {
         for (int w = 0; w < width; w++) {
            *(d + p++) = Y;
         }
      }
      for (int h = 0; h < height / 4; h++) {
         for (int w = 0; w < width; w++) {
            *(d + p++) = U;
            *(d + p++) = V;
         }
      }

      if (imageSize > 0) {
         memcpy(StrFmPtr.getRawPointer(), d, imageSize);
      }

      sH264Data->outputResults[currentFrame].Result = StrFmPtr;
      sH264Data->outputResults[currentFrame].ResultWidth = (int32_t)width;
      sH264Data->outputResults[currentFrame].ResultHeight = (int32_t)height;

      currentFrame++;
   }
#endif

   TriggerCallback();
   
   return 0X000000E4;
}

void 
TriggerCallback()
{
   if (sH264Data->callback) {
      if (sH264Data->buffered) {
         if (currentFrame >= MAX_FRAMES) {

            // Very inefficient way of sorting frames by time stamp
            double lastTimeStamp = 0.0;

            for (int i = 0; i < MAX_FRAMES; ++i) {
               int idx = GetNextIndex(lastTimeStamp);
               lastTimeStamp = sH264Data->outputResults[idx].TimeStamp;
               copy(outp->DecResPtr[i], sH264Data->outputResults[idx]);
            }

            outp->FmCnt = currentFrame;
            auto result = cafe::invoke(cpu::this_core::state(),
               sH264Data->callback,
               outp);
            if (result) {
               gLog->error("H264 - Error invoking callback resut = {}", result);
            }

            currentFrame = 0;
         }
      } else {
         outp->FmCnt = 1;
         cafe::invoke(cpu::this_core::state(),
            sH264Data->callback,
            virt_cast<void *>(outp));
         currentFrame = 0;
      }
   } else {
      currentFrame = 0;
   }
}

int32_t 
GetNextIndex(double currentTimeStamp)
{
   int32_t index = -1;
   double minTimeStamp = 99999.0;
   for (int i = 0; i < MAX_FRAMES; ++i) {
      if (sH264Data->outputResults[i].TimeStamp > currentTimeStamp + 0.0001) {
         if (sH264Data->outputResults[i].TimeStamp < minTimeStamp) {
            minTimeStamp = sH264Data->outputResults[i].TimeStamp;
            index = i;
         }
      }   
   }
   return index;
}

int32_t
H264DECEnd(virt_ptr<void> memPtr)
{
   TriggerCallback();
   return 0;
}

int32_t
H264DECFlush(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECClose(virt_ptr<void> memPtr)
{
   decaf_warn_stub();
   return 0;
}


#ifdef FFMPEG_DEC
int32_t 
ffmpeg_encoder_init_frame(AVFrame **framep,
                          AVCodecContext* context,
                          int width,
                          int height)
{
   AVFrame *frame = av_frame_alloc();

   if (!frame) {
      gLog->error("H264: Could not allocate video frame");
      return -1;
   }

   frame->format = context->pix_fmt;
   frame->width = width;
   frame->height = height;

   *framep = frame;

   return 0;
}
#endif

static uint32_t
getLevelMemoryRequirement(int32_t level)
{
   switch (level) {
   case 10:
      return 0x63000;
   case 11:
      return 0xE1000;
   case 12:
   case 13:
   case 20:
      return 0x252000;
   case 21:
      return 0x4A4000;
   case 22:
   case 30:
      return 0x7E9000;
   case 31:
      return 0x1194000;
   case 32:
      return 0x1400000;
   case 40:
   case 41:
      return 0x2000000;
   case 42:
      return 0x2200000;
   case 50:
      return 0x6BD0000;
   case 51:
      return 0xB400000;
   default:
      decaf_abort(fmt::format("Unexpected H264 level {}", level));
   }
}

int32_t
H264DECMemoryRequirement(int32_t profile,
                         int32_t level,
                         int32_t maxWidth,
                         int32_t maxHeight,
                         virt_ptr<uint32_t> codecMemSize)
{
   switch (profile)
   {
   case 66:  // Constrained  Baseline Profile
      break;
   case 77:  // Main Profile
      break;
   case 100: // High Profile
      break;
   default:
      decaf_abort(fmt::format("Unsupported profile {}", profile));
      break;
   }

   *codecMemSize = BaseMemoryRequirement + getLevelMemoryRequirement(level) + 1023u;

   width = maxWidth;
   height = maxHeight;

   return 0;
}

int32_t
H264DECGetImageSize(const virt_ptr <uint8_t> buf,
                   int32_t TotalBytes,
                   int32_t streamOffset,
                   virt_ptr<int32_t> Width,
                   virt_ptr <int32_t> Height)
{
#ifdef FFMPEG_DEC
   uint8_t* b = (uint8_t*)buf.getRawPointer();
   b++;
   b++;
   bs_t* bs = bs_new(b, TotalBytes - 2);
   read_seq_parameter_set_rbsp(h, bs);

   for (int i = 0; i < MAX_FRAMES; ++i) {
      sH264Data->outputResults[i].TopCrop = h->sps->frame_crop_top_offset;
      sH264Data->outputResults[i].BottomCrop = h->sps->frame_crop_bottom_offset;
      sH264Data->outputResults[i].LeftCrop = h->sps->frame_crop_left_offset;
      sH264Data->outputResults[i].RightCrop = h->sps->frame_crop_right_offset;

      if (h->sps->vui_parameters_present_flag) {
         sH264Data->outputResults[i].vui_parameters_present_flag = (uint8_t)h->sps->vui_parameters_present_flag;
         sH264Data->outputResults[i].VUIparameters.aspect_ratio_info_present_flag = (uint8_t)h->sps->vui.aspect_ratio_info_present_flag;
         sH264Data->outputResults[i].VUIparameters.aspect_ratio_idc = (uint8_t)h->sps->vui.aspect_ratio_idc;
         sH264Data->outputResults[i].VUIparameters.sar_width = (int16_t)h->sps->vui.sar_width;
         sH264Data->outputResults[i].VUIparameters.sar_height = (int16_t)h->sps->vui.sar_height;
         sH264Data->outputResults[i].VUIparameters.overscan_info_present_flag = (uint8_t)h->sps->vui.overscan_info_present_flag;
         sH264Data->outputResults[i].VUIparameters.overscan_appropriate_flag = (uint8_t)h->sps->vui.overscan_appropriate_flag;
         sH264Data->outputResults[i].VUIparameters.video_signal_type_present_flag = (uint8_t)h->sps->vui.video_signal_type_present_flag;
         sH264Data->outputResults[i].VUIparameters.video_format = (uint8_t)h->sps->vui.video_format;
         sH264Data->outputResults[i].VUIparameters.video_full_range_flag = (uint8_t)h->sps->vui.video_full_range_flag;
         sH264Data->outputResults[i].VUIparameters.colour_description_present_flag = (uint8_t)h->sps->vui.colour_description_present_flag;
         sH264Data->outputResults[i].VUIparameters.colour_primaries = (uint8_t)h->sps->vui.colour_primaries;
         sH264Data->outputResults[i].VUIparameters.transfer_characteristics = (uint8_t)h->sps->vui.transfer_characteristics;
         sH264Data->outputResults[i].VUIparameters.matrix_coefficients = (uint8_t)h->sps->vui.matrix_coefficients;
         sH264Data->outputResults[i].VUIparameters.chroma_loc_info_present_flag = (uint8_t)h->sps->vui.chroma_loc_info_present_flag;
         sH264Data->outputResults[i].VUIparameters.chroma_sample_loc_type_top_field = (uint8_t)h->sps->vui.chroma_sample_loc_type_top_field;
         sH264Data->outputResults[i].VUIparameters.chroma_sample_loc_type_bottom_field = (uint8_t)h->sps->vui.chroma_sample_loc_type_bottom_field;
         sH264Data->outputResults[i].VUIparameters.timing_info_present_flag = (uint8_t)h->sps->vui.timing_info_present_flag;
         sH264Data->outputResults[i].VUIparameters.num_units_in_tick = (uint32_t)h->sps->vui.num_units_in_tick;
         sH264Data->outputResults[i].VUIparameters.time_scale = (uint32_t)h->sps->vui.time_scale;
         sH264Data->outputResults[i].VUIparameters.fixed_frame_rate_flag = (uint8_t)h->sps->vui.fixed_frame_rate_flag;
         sH264Data->outputResults[i].VUIparameters.nal_hrd_parameters_present_flag = (uint8_t)h->sps->vui.nal_hrd_parameters_present_flag;
         sH264Data->outputResults[i].VUIparameters.vcl_hrd_parameters_present_flag = (uint8_t)h->sps->vui.vcl_hrd_parameters_present_flag;
         sH264Data->outputResults[i].VUIparameters.low_delay_hrd_flag = (uint8_t)h->sps->vui.low_delay_hrd_flag;
         sH264Data->outputResults[i].VUIparameters.pic_struct_present_flag = (uint8_t)h->sps->vui.pic_struct_present_flag;
         sH264Data->outputResults[i].VUIparameters.bitstream_restriction_flag = (uint8_t)h->sps->vui.bitstream_restriction_flag;
         sH264Data->outputResults[i].VUIparameters.motion_vectors_over_pic_boundaries_flag = (uint8_t)h->sps->vui.motion_vectors_over_pic_boundaries_flag;
         sH264Data->outputResults[i].VUIparameters.max_bytes_per_pic_denom = (int16_t)h->sps->vui.max_bytes_per_pic_denom;
         sH264Data->outputResults[i].VUIparameters.max_bits_per_mb_denom = (int16_t)h->sps->vui.max_bits_per_mb_denom;
         sH264Data->outputResults[i].VUIparameters.log2_max_mv_length_horizontal = (int16_t)h->sps->vui.log2_max_mv_length_horizontal;
         sH264Data->outputResults[i].VUIparameters.log2_max_mv_length_vertical = (int16_t)h->sps->vui.log2_max_mv_length_vertical;
         sH264Data->outputResults[i].VUIparameters.num_reorder_frames = (int16_t)h->sps->vui.num_reorder_frames;
         sH264Data->outputResults[i].VUIparameters.max_dec_frame_buffering = (int16_t)h->sps->vui.max_dec_frame_buffering;
      }
   }
   *Width = (h->sps->pic_width_in_mbs_minus1 + 1) * 16;
   *Height = (h->sps->pic_height_in_map_units_minus1 + 1) * 16;

   if (width != *Width || height != *Height)
   {
      width = *Width;
      height = *Height;

      frame->width = width;
      frame->height = height;

      sws = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV420P, frame->width, frame->height, AV_PIX_FMT_NV12, 2, NULL, NULL, NULL);

      if (ffmpeg_encoder_init_frame(&frame2, context, frame->width, frame->height)) {
         gLog->error("H264DECGetImageSize({},{},{},{},{}) -> Frame failed to decoded successfully (ffmpeg_encoder_init_frame)", buf, TotalBytes, streamOffset, *Width, Height);
         return -1;
      }
   }
#else
   *Width = width;
   *Height = height;
#endif
   return 0;
}

int32_t
H264DECFindDecstartpoint(const virt_ptr<uint8_t> buf,
                         int32_t totalBytes,
                         virt_ptr <int32_t> streamOffset)
{
   if (!buf || totalBytes < 4 || !streamOffset) {
      return -1;
   }

   for (auto i = 0; i < totalBytes - 4; i++) {
      auto start = buf[i];
      if (buf[i + 0] == 0 &&
         buf[i + 1] == 0 &&
         buf[i + 2] == 1 &&
         (buf[i + 3] & H264SliceTypeMask) == H264NalSps) {
         *streamOffset = ((i - 1) < 0) ? 0 : (i - 1);
         return 0;
      }
   }
   return -1;
}

int32_t
H264DECFindIdrpoint(const virt_ptr <uint8_t> buf,
                    int32_t totalBytes,
                    virt_ptr <int32_t> streamOffset)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECCheckDecunitLength(virt_ptr<void> memPtr,
                         const virt_ptr<uint8_t> buf,
                         int32_t totalBytes,
                         int32_t streamOffset,
                         virt_ptr<int32_t>length)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECCheckMemSegmentation(virt_ptr<void> memPtr,
                            uint32_t Size)
{
   decaf_warn_stub();
   return 0;
}

int32_t
H264DECCheckSkipableFrame(const virt_ptr<uint8_t> buf,
                          int32_t Length,
                          virt_ptr<int32_t>SkipFlag)
{
   decaf_warn_stub();
   return 0;
}


void 
copy(H264DECResult &out, 
     H264DECResult in)
{
   out.DecStatus = in.DecStatus;
   out.TimeStamp = in.TimeStamp;
   out.ResultWidth = in.ResultWidth;
   out.ResultHeight = in.ResultHeight;
   out.NextLine = in.NextLine;
   out.CropEnableFlag = in.CropEnableFlag;
   out.TopCrop = in.TopCrop;
   out.BottomCrop = in.BottomCrop;
   out.LeftCrop = in.LeftCrop;
   out.RightCrop = in.RightCrop;
   out.PanScanEnableFlag = in.PanScanEnableFlag;
   out.TopPanScan = in.TopPanScan;
   out.BottomPanScan = in.BottomPanScan;
   out.LeftPanScan = in.LeftPanScan;
   out.RightPanScan = in.RightPanScan;
   out.Result = in.Result;
   out.vui_parameters_present_flag = in.vui_parameters_present_flag;

   out.VUIparameters.aspect_ratio_info_present_flag = in.VUIparameters.aspect_ratio_info_present_flag;
   out.VUIparameters.aspect_ratio_idc = in.VUIparameters.aspect_ratio_idc;
   out.VUIparameters.sar_width = in.VUIparameters.sar_width;
   out.VUIparameters.sar_height = in.VUIparameters.sar_height;
   out.VUIparameters.overscan_info_present_flag = in.VUIparameters.overscan_info_present_flag;
   out.VUIparameters.overscan_appropriate_flag = in.VUIparameters.overscan_appropriate_flag;
   out.VUIparameters.video_signal_type_present_flag = in.VUIparameters.video_signal_type_present_flag;
   out.VUIparameters.video_format = in.VUIparameters.video_format;
   out.VUIparameters.video_full_range_flag = in.VUIparameters.video_full_range_flag;
   out.VUIparameters.colour_description_present_flag = in.VUIparameters.colour_description_present_flag;
   out.VUIparameters.colour_primaries = in.VUIparameters.colour_primaries;
   out.VUIparameters.transfer_characteristics = in.VUIparameters.transfer_characteristics;
   out.VUIparameters.matrix_coefficients = in.VUIparameters.matrix_coefficients;
   out.VUIparameters.chroma_loc_info_present_flag = in.VUIparameters.chroma_loc_info_present_flag;
   out.VUIparameters.chroma_sample_loc_type_top_field = in.VUIparameters.chroma_sample_loc_type_top_field;
   out.VUIparameters.chroma_sample_loc_type_bottom_field = in.VUIparameters.chroma_sample_loc_type_bottom_field;
   out.VUIparameters.timing_info_present_flag = in.VUIparameters.timing_info_present_flag;
   out.VUIparameters.num_units_in_tick = in.VUIparameters.num_units_in_tick;
   out.VUIparameters.time_scale = in.VUIparameters.time_scale;
   out.VUIparameters.fixed_frame_rate_flag = in.VUIparameters.fixed_frame_rate_flag;
   out.VUIparameters.nal_hrd_parameters_present_flag = in.VUIparameters.nal_hrd_parameters_present_flag;
   out.VUIparameters.vcl_hrd_parameters_present_flag = in.VUIparameters.vcl_hrd_parameters_present_flag;
   out.VUIparameters.low_delay_hrd_flag = in.VUIparameters.low_delay_hrd_flag;
   out.VUIparameters.pic_struct_present_flag = in.VUIparameters.pic_struct_present_flag;
   out.VUIparameters.bitstream_restriction_flag = in.VUIparameters.bitstream_restriction_flag;
   out.VUIparameters.motion_vectors_over_pic_boundaries_flag = in.VUIparameters.motion_vectors_over_pic_boundaries_flag;
   out.VUIparameters.max_bytes_per_pic_denom = in.VUIparameters.max_bytes_per_pic_denom;
   out.VUIparameters.max_bits_per_mb_denom = in.VUIparameters.max_bits_per_mb_denom;
   out.VUIparameters.log2_max_mv_length_horizontal = in.VUIparameters.log2_max_mv_length_horizontal;
   out.VUIparameters.log2_max_mv_length_vertical = in.VUIparameters.log2_max_mv_length_vertical;
   out.VUIparameters.num_reorder_frames = in.VUIparameters.num_reorder_frames;
   out.VUIparameters.max_dec_frame_buffering = in.VUIparameters.max_dec_frame_buffering;
}

void
Library::registerDecSymbols()
{
   RegisterFunctionExport(H264DECInitParam);
   RegisterFunctionExport(H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_FPTR_OUTPUT", H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_OUTPUT_PER_FRAME", H264DECSetParam);
   RegisterFunctionExportName("H264DECSetParam_USER_MEMORY", H264DECSetParam);
   RegisterFunctionExport(H264DECOpen);
   RegisterFunctionExport(H264DECBegin);
   RegisterFunctionExport(H264DECSetBitstream);
   RegisterFunctionExport(H264DECExecute);
   RegisterFunctionExport(H264DECEnd);
   RegisterFunctionExport(H264DECFlush);
   RegisterFunctionExport(H264DECClose);

   RegisterFunctionExport(H264DECMemoryRequirement);
   RegisterFunctionExport(H264DECGetImageSize);
   RegisterFunctionExport(H264DECFindDecstartpoint);
   RegisterFunctionExport(H264DECFindIdrpoint);
   RegisterFunctionExport(H264DECCheckDecunitLength);
   RegisterFunctionExport(H264DECCheckMemSegmentation);
   RegisterFunctionExport(H264DECCheckSkipableFrame);

   RegisterDataInternal(outp);
   RegisterDataInternal(sH264Data);
}

} // namespace cafe::h264_dec