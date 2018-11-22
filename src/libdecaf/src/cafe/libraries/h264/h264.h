#pragma once
#include "cafe/libraries/cafe_hle_library.h"

//#define FFMPEG_DEC
#define MAX_FRAMES  5

#ifdef  FFMPEG_DEC
extern "C"
{
#include <..\..\..\..\..\libraries\ffmpeg\include\avcodec.h>
#include <..\..\..\..\..\libraries\ffmpeg\include\avformat.h>
#include <..\..\..\..\..\libraries\ffmpeg\include\swscale.h>
}
#endif


namespace cafe::h264
{
using  H264Callback = virt_func_ptr<
   int32_t(virt_ptr<void>)>;

#pragma pack(push, 1)
struct VUIParameters
{
   be2_val<uint8_t> aspect_ratio_info_present_flag;
   be2_val<uint8_t> aspect_ratio_idc;
   be2_val<int16_t> sar_width;
   be2_val<int16_t> sar_height;
   be2_val<uint8_t> overscan_info_present_flag;
   be2_val<uint8_t> overscan_appropriate_flag;
   be2_val<uint8_t> video_signal_type_present_flag;
   be2_val<uint8_t> video_format;
   be2_val<uint8_t> video_full_range_flag;
   be2_val<uint8_t> colour_description_present_flag;
   be2_val<uint8_t> colour_primaries;
   be2_val<uint8_t> transfer_characteristics;
   be2_val<uint8_t> matrix_coefficients;
   be2_val<uint8_t> chroma_loc_info_present_flag;
   be2_val<uint8_t> chroma_sample_loc_type_top_field;
   be2_val<uint8_t> chroma_sample_loc_type_bottom_field;
   be2_val<uint8_t> timing_info_present_flag;
   be2_val<uint32_t> num_units_in_tick;
   be2_val<uint32_t> time_scale;
   be2_val<uint8_t> fixed_frame_rate_flag;
   be2_val<uint8_t> nal_hrd_parameters_present_flag;
   be2_val<uint8_t> vcl_hrd_parameters_present_flag;
   be2_val<uint8_t> low_delay_hrd_flag;
   be2_val<uint8_t> pic_struct_present_flag;
   be2_val<uint8_t> bitstream_restriction_flag;
   be2_val<uint8_t> motion_vectors_over_pic_boundaries_flag;
   be2_val<int16_t> max_bytes_per_pic_denom;
   be2_val<int16_t> max_bits_per_mb_denom;
   be2_val<int16_t> log2_max_mv_length_horizontal;
   be2_val<int16_t> log2_max_mv_length_vertical;
   be2_val<int16_t> num_reorder_frames;
   be2_val<int16_t> max_dec_frame_buffering;
};

struct H264DECResult
{
   be2_val<int32_t> DecStatus;
   be2_val<double> TimeStamp;
   be2_val<int32_t> ResultWidth;
   be2_val<int32_t> ResultHeight;
   be2_val<int32_t> NextLine;
   be2_val<uint8_t> CropEnableFlag;
   be2_val<int32_t> TopCrop;
   be2_val<int32_t> BottomCrop;
   be2_val<int32_t> LeftCrop;
   be2_val<int32_t> RightCrop;
   be2_val<uint8_t> PanScanEnableFlag;
   be2_val<int32_t> TopPanScan;
   be2_val<int32_t> BottomPanScan;
   be2_val<int32_t> LeftPanScan;
   be2_val<int32_t> RightPanScan;
   be2_virt_ptr<void>    Result;
   be2_val<uint8_t> vui_parameters_present_flag;
   be2_struct<VUIParameters> VUIparameters;
   be2_array<int32_t, 10> reserved;
};

struct H264DECOutput
{
   be2_val<int32_t> FmCnt;
   be2_array<be2_struct<H264DECResult>, MAX_FRAMES> DecResPtr;
   be2_virt_ptr<void> UserMemory;
};

struct H264Data
{
   be2_val<H264Callback> callback;
   be2_struct<H264DECOutput> output;   
   be2_array<be2_struct<H264DECResult>, MAX_FRAMES> outputResults;
   be2_val<BOOL> buffered;
};


#ifdef  FFMPEG_DEC
static AVCodecContext* context;
static AVFrame* frame;
static AVPacket avpkt;
static SwsContext* sws;
static AVFrame* frame2;
#endif

static int32_t currentFrame;
static int32_t width;
static int32_t height;

#pragma pack(pop)

static virt_ptr<H264Data>
   sH264Data;

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::h264, "h264.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerDecSymbols();
};

} // namespace cafe::h264
