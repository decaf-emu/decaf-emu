/* 
 * h264bitstream - a library for reading and writing H.264 video
 * Copyright (C) 2005-2007 Auroras Entertainment, LLC
 * Copyright (C) 2008-2011 Avail-TVN
 * 
 * Written by Alex Izvorski <aizvorski@gmail.com> and Alex Giladi <alex.giladi@gmail.com>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "bs.h"
#include "h264_stream.h"
#include "h264_sei.h"

FILE* h264_dbgfile = NULL;

#define printf(...) fprintf((h264_dbgfile == NULL ? stdout : h264_dbgfile), __VA_ARGS__)

/** 
 Calculate the log base 2 of the argument, rounded up. 
 Zero or negative arguments return zero 
 Idea from http://www.southwindsgames.com/blog/2009/01/19/fast-integer-log2-function-in-cc/
 */
int intlog2(int x)
{
    int log = 0;
    if (x < 0) { x = 0; }
    while ((x >> log) > 0)
    {
        log++;
    }
    if (log > 0 && x == 1<<(log-1)) { log--; }
    return log;
}

int is_slice_type(int slice_type, int cmp_type)
{
    if (slice_type >= 5) { slice_type -= 5; }
    if (cmp_type >= 5) { cmp_type -= 5; }
    if (slice_type == cmp_type) { return 1; }
    else { return 0; }
}

/***************************** reading ******************************/

/**
 Create a new H264 stream object.  Allocates all structures contained within it.
 @return    the stream object
 */
h264_stream_t* h264_new()
{
    h264_stream_t* h = (h264_stream_t*)calloc(1, sizeof(h264_stream_t));

    h->nal = (nal_t*)calloc(1, sizeof(nal_t));

    // initialize tables
    for ( int i = 0; i < 32; i++ ) { h->sps_table[i] = (sps_t*)calloc(1, sizeof(sps_t)); }
    for ( int i = 0; i < 256; i++ ) { h->pps_table[i] = (pps_t*)calloc(1, sizeof(pps_t)); }

    h->sps = h->sps_table[0];
    h->pps = h->pps_table[0];
    h->aud = (aud_t*)calloc(1, sizeof(aud_t));
    h->num_seis = 0;
    h->seis = NULL;
    h->sei = NULL;  //This is a TEMP pointer at whats in h->seis...
    h->sh = (slice_header_t*)calloc(1, sizeof(slice_header_t));

    return h;   
}


/**
 Free an existing H264 stream object.  Frees all contained structures.
 @param[in,out] h   the stream object
 */
void h264_free(h264_stream_t* h)
{
    free(h->nal);

    for ( int i = 0; i < 32; i++ ) { free( h->sps_table[i] ); }
    for ( int i = 0; i < 256; i++ ) { free( h->pps_table[i] ); }

    free(h->aud);
    if(h->seis != NULL)
    {
        for( int i = 0; i < h->num_seis; i++ )
        {
            sei_t* sei = h->seis[i];
            sei_free(sei);
        }
        free(h->seis);
    }
    free(h->sh);
    free(h);
}

/**
 Find the beginning and end of a NAL (Network Abstraction Layer) unit in a byte buffer containing H264 bitstream data.
 @param[in]   buf        the buffer
 @param[in]   size       the size of the buffer
 @param[out]  nal_start  the beginning offset of the nal
 @param[out]  nal_end    the end offset of the nal
 @return                 the length of the nal, or 0 if did not find start of nal, or -1 if did not find end of nal
 */
// DEPRECATED - this will be replaced by a similar function with a slightly different API
int find_nal_unit(uint8_t* buf, int size, int* nal_start, int* nal_end)
{
    int i;
    // find start
    *nal_start = 0;
    *nal_end = 0;
    
    i = 0;
    while (   //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) && 
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0 || buf[i+3] != 0x01) 
        )
    {
        i++; // skip leading zero
        if (i+4 >= size) { return 0; } // did not find nal start
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) // ( next_bits( 24 ) != 0x000001 )
    {
        i++;
    }

    if  (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) { /* error, should never happen */ return 0; }
    i+= 3;
    *nal_start = i;
    
    while (   //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0) && 
        (buf[i] != 0 || buf[i+1] != 0 || buf[i+2] != 0x01) 
        )
    {
        i++;
        // FIXME the next line fails when reading a nal that ends exactly at the end of the data
        if (i+3 >= size) { *nal_end = size; return -1; } // did not find nal end, stream ended first
    }
    
    *nal_end = i;
    return (*nal_end - *nal_start);
}


int more_rbsp_data(h264_stream_t* h, bs_t* b) 
{
    if ( bs_eof(b) ) { return 0; }
    if ( bs_peek_u1(b) == 1 ) { return 0; } // if next bit is 1, we've reached the stop bit
    return 1;
}

/**
   Convert RBSP data to NAL data (Annex B format).
   The size of nal_buf must be 4/3 * the size of the rbsp_buf (rounded up) to guarantee the output will fit.
   If that is not true, output may be truncated and an error will be returned.
   If that is true, there is no possible error during this conversion.
   @param[in] rbsp_buf   the rbsp data
   @param[in] rbsp_size  pointer to the size of the rbsp data
   @param[in,out] nal_buf   allocated memory in which to put the nal data
   @param[in,out] nal_size  as input, pointer to the maximum size of the nal data; as output, filled in with the actual size of the nal data
   @return  actual size of nal data, or -1 on error
 */
// 7.3.1 NAL unit syntax
// 7.4.1.1 Encapsulation of an SODB within an RBSP
int rbsp_to_nal(const uint8_t* rbsp_buf, const int* rbsp_size, uint8_t* nal_buf, int* nal_size)
{
    int i;
    int j     = 1;
    int count = 0;

    if (*nal_size > 0) { nal_buf[0] = 0x00; } // zero out first byte since we start writing from second byte

    for ( i = 0; i < *rbsp_size ; i++ )
    {
        if ( j >= *nal_size ) 
        {
            // error, not enough space
            return -1;
        }

        if ( ( count == 2 ) && !(rbsp_buf[i] & 0xFC) ) // HACK 0xFC
        {
            nal_buf[j] = 0x03;
            j++;
            count = 0;
        }
        nal_buf[j] = rbsp_buf[i];
        if ( rbsp_buf[i] == 0x00 )
        {
            count++;
        }
        else
        {
            count = 0;
        }
        j++;
    }

    *nal_size = j;
    return j;
}

/**
   Convert NAL data (Annex B format) to RBSP data.
   The size of rbsp_buf must be the same as size of the nal_buf to guarantee the output will fit.
   If that is not true, output may be truncated and an error will be returned. 
   Additionally, certain byte sequences in the input nal_buf are not allowed in the spec and also cause the conversion to fail and an error to be returned.
   @param[in] nal_buf   the nal data
   @param[in,out] nal_size  as input, pointer to the size of the nal data; as output, filled in with the actual size of the nal data
   @param[in,out] rbsp_buf   allocated memory in which to put the rbsp data
   @param[in,out] rbsp_size  as input, pointer to the maximum size of the rbsp data; as output, filled in with the actual size of rbsp data
   @return  actual size of rbsp data, or -1 on error
 */
// 7.3.1 NAL unit syntax
// 7.4.1.1 Encapsulation of an SODB within an RBSP
int nal_to_rbsp(const uint8_t* nal_buf, int* nal_size, uint8_t* rbsp_buf, int* rbsp_size)
{
    int i;
    int j     = 0;
    int count = 0;
  
    for( i = 1; i < *nal_size; i++ )
    { 
        // in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
        if( ( count == 2 ) && ( nal_buf[i] < 0x03) ) 
        {
            return -1;
        }

        if( ( count == 2 ) && ( nal_buf[i] == 0x03) )
        {
            // check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
            if((i < *nal_size - 1) && (nal_buf[i+1] > 0x03))
            {
                return -1;
            }

            // if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
            if(i == *nal_size - 1)
            {
                break;
            }

            i++;
            count = 0;
        }

        if ( j >= *rbsp_size ) 
        {
            // error, not enough space
            return -1;
        }

        rbsp_buf[j] = nal_buf[i];
        if(nal_buf[i] == 0x00)
        {
            count++;
        }
        else
        {
            count = 0;
        }
        j++;
    }

    *nal_size = i;
    *rbsp_size = j;
    return j;
}

/**
 Read a NAL unit from a byte buffer.
 The buffer must start exactly at the beginning of the nal (after the start prefix).
 The NAL is read into h->nal and into other fields within h depending on its type (check h->nal->nal_unit_type after reading).
 @param[in,out] h          the stream object
 @param[in]     buf        the buffer
 @param[in]     size       the size of the buffer
 @return                   the length of data actually read, or -1 on error
 */
//7.3.1 NAL unit syntax
int read_nal_unit(h264_stream_t* h, uint8_t* buf, int size)
{
    nal_t* nal = h->nal;

    bs_t* b = bs_new(buf, size);

    nal->forbidden_zero_bit = bs_read_f(b,1);
    nal->nal_ref_idc = bs_read_u(b,2);
    nal->nal_unit_type = bs_read_u(b,5);
    nal->parsed = NULL;
    nal->sizeof_parsed = 0;

    bs_free(b);

    int nal_size = size;
    int rbsp_size = size;
    uint8_t* rbsp_buf = (uint8_t*)malloc(rbsp_size);
 
    int rc = nal_to_rbsp(buf, &nal_size, rbsp_buf, &rbsp_size);

    if (rc < 0) { free(rbsp_buf); return -1; } // handle conversion error

    b = bs_new(rbsp_buf, rbsp_size);

    switch ( nal->nal_unit_type )
    {
        case NAL_UNIT_TYPE_CODED_SLICE_IDR:
        case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:  
        case NAL_UNIT_TYPE_CODED_SLICE_AUX:
            read_slice_layer_rbsp(h, b);
            nal->parsed = h->sh;
            nal->sizeof_parsed = sizeof(slice_header_t);
            break;

        case NAL_UNIT_TYPE_SEI:
            read_sei_rbsp(h, b);
            nal->parsed = h->sei;
            nal->sizeof_parsed = sizeof(sei_t);
            break;

        case NAL_UNIT_TYPE_SPS: 
            read_seq_parameter_set_rbsp(h, b); 
            nal->parsed = h->sps;
            nal->sizeof_parsed = sizeof(sps_t);
            break;

        case NAL_UNIT_TYPE_PPS:   
            read_pic_parameter_set_rbsp(h, b);
            nal->parsed = h->pps;
            nal->sizeof_parsed = sizeof(pps_t);
            break;

        case NAL_UNIT_TYPE_AUD:     
            read_access_unit_delimiter_rbsp(h, b); 
            nal->parsed = h->aud;
            nal->sizeof_parsed = sizeof(aud_t);
            break;

        case NAL_UNIT_TYPE_END_OF_SEQUENCE: 
            read_end_of_seq_rbsp(h, b);
            break;

        case NAL_UNIT_TYPE_END_OF_STREAM: 
            read_end_of_stream_rbsp(h, b);
            break;
        //case NAL_UNIT_TYPE_FILLER:
        //case NAL_UNIT_TYPE_SPS_EXT:
        //case NAL_UNIT_TYPE_UNSPECIFIED:
        //case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A:  
        //case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B: 
        //case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C:
        default:
            // here comes the reserved/unspecified/ignored stuff
            nal->parsed = NULL;
            nal->sizeof_parsed = 0;
            return 0;
    }

    if (bs_overrun(b)) { bs_free(b); free(rbsp_buf); return -1; }

    bs_free(b); 
    free(rbsp_buf);

    return nal_size;
}

/**
 Read only the NAL headers (enough to determine unit type) from a byte buffer.
 @return unit type if read successfully, or -1 if this doesn't look like a nal
*/
int peek_nal_unit(h264_stream_t* h, uint8_t* buf, int size)
{
    nal_t* nal = h->nal;

    bs_t* b = bs_new(buf, size);

    nal->forbidden_zero_bit = bs_read_f(b,1);
    nal->nal_ref_idc = bs_read_u(b,2);
    nal->nal_unit_type = bs_read_u(b,5);

    bs_free(b);

    // basic verification, per 7.4.1
    if ( nal->forbidden_zero_bit ) { return -1; }
    if ( nal->nal_unit_type <= 0 || nal->nal_unit_type > 20 ) { return -1; }
    if ( nal->nal_unit_type > 15 && nal->nal_unit_type < 19 ) { return -1; }

    if ( nal->nal_ref_idc == 0 )
    {
        if ( nal->nal_unit_type == NAL_UNIT_TYPE_CODED_SLICE_IDR )
        {
            return -1;
        }
    }
    else 
    {
        if ( nal->nal_unit_type ==  NAL_UNIT_TYPE_SEI || 
             nal->nal_unit_type == NAL_UNIT_TYPE_AUD || 
             nal->nal_unit_type == NAL_UNIT_TYPE_END_OF_SEQUENCE || 
             nal->nal_unit_type == NAL_UNIT_TYPE_END_OF_STREAM || 
             nal->nal_unit_type == NAL_UNIT_TYPE_FILLER ) 
        {
            return -1;
        }
    }

    return nal->nal_unit_type;
}

//7.3.2.1 Sequence parameter set RBSP syntax
void read_seq_parameter_set_rbsp(h264_stream_t* h, bs_t* b)
{
    int i;

    // NOTE can't read directly into sps because seq_parameter_set_id not yet known and so sps is not selected

    int profile_idc = bs_read_u8(b);
    int constraint_set0_flag = bs_read_u1(b);
    int constraint_set1_flag = bs_read_u1(b);
    int constraint_set2_flag = bs_read_u1(b);
    int constraint_set3_flag = bs_read_u1(b);
    int constraint_set4_flag = bs_read_u1(b);
    int constraint_set5_flag = bs_read_u1(b);
    int reserved_zero_2bits  = bs_read_u(b,2);  /* all 0's */
    int level_idc = bs_read_u8(b);
    int seq_parameter_set_id = bs_read_ue(b);

    // select the correct sps
    h->sps = h->sps_table[seq_parameter_set_id];
    sps_t* sps = h->sps;
    memset(sps, 0, sizeof(sps_t));
    
    sps->chroma_format_idc = 1; 

    sps->profile_idc = profile_idc; // bs_read_u8(b);
    sps->constraint_set0_flag = constraint_set0_flag;//bs_read_u1(b);
    sps->constraint_set1_flag = constraint_set1_flag;//bs_read_u1(b);
    sps->constraint_set2_flag = constraint_set2_flag;//bs_read_u1(b);
    sps->constraint_set3_flag = constraint_set3_flag;//bs_read_u1(b);
    sps->constraint_set4_flag = constraint_set4_flag;//bs_read_u1(b);
    sps->constraint_set5_flag = constraint_set5_flag;//bs_read_u1(b);
    sps->reserved_zero_2bits = reserved_zero_2bits;//bs_read_u(b,2);
    sps->level_idc = level_idc; //bs_read_u8(b);
    sps->seq_parameter_set_id = seq_parameter_set_id; // bs_read_ue(b);
    if( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 144 )
    {
        sps->chroma_format_idc = bs_read_ue(b);
        if( sps->chroma_format_idc == 3 )
        {
            sps->residual_colour_transform_flag = bs_read_u1(b);
        }
        sps->bit_depth_luma_minus8 = bs_read_ue(b);
        sps->bit_depth_chroma_minus8 = bs_read_ue(b);
        sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
        sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
        if( sps->seq_scaling_matrix_present_flag )
        {
            for( i = 0; i < 8; i++ )
            {
                sps->seq_scaling_list_present_flag[ i ] = bs_read_u1(b);
                if( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        read_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                      sps->UseDefaultScalingMatrix4x4Flag[ i ]);
                    }
                    else
                    {
                        read_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                      sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
    }
    sps->log2_max_frame_num_minus4 = bs_read_ue(b);
    sps->pic_order_cnt_type = bs_read_ue(b);
    if( sps->pic_order_cnt_type == 0 )
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);
    }
    else if( sps->pic_order_cnt_type == 1 )
    {
        sps->delta_pic_order_always_zero_flag = bs_read_u1(b);
        sps->offset_for_non_ref_pic = bs_read_se(b);
        sps->offset_for_top_to_bottom_field = bs_read_se(b);
        sps->num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(b);
        for( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            sps->offset_for_ref_frame[ i ] = bs_read_se(b);
        }
    }
    sps->num_ref_frames = bs_read_ue(b);
    sps->gaps_in_frame_num_value_allowed_flag = bs_read_u1(b);
    sps->pic_width_in_mbs_minus1 = bs_read_ue(b);
    sps->pic_height_in_map_units_minus1 = bs_read_ue(b);
    sps->frame_mbs_only_flag = bs_read_u1(b);
    if( !sps->frame_mbs_only_flag )
    {
        sps->mb_adaptive_frame_field_flag = bs_read_u1(b);
    }
    sps->direct_8x8_inference_flag = bs_read_u1(b);
    sps->frame_cropping_flag = bs_read_u1(b);
    if( sps->frame_cropping_flag )
    {
        sps->frame_crop_left_offset = bs_read_ue(b);
        sps->frame_crop_right_offset = bs_read_ue(b);
        sps->frame_crop_top_offset = bs_read_ue(b);
        sps->frame_crop_bottom_offset = bs_read_ue(b);
    }
    sps->vui_parameters_present_flag = bs_read_u1(b);
    if( sps->vui_parameters_present_flag )
    {
        read_vui_parameters(h, b);
    }
    read_rbsp_trailing_bits(h, b);
}


//7.3.2.1.1 Scaling list syntax
void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag )
{
    int j;
    if(scalingList == NULL)
    {
        return;
    }

    int lastScale = 8;
    int nextScale = 8;
    for( j = 0; j < sizeOfScalingList; j++ )
    {
        if( nextScale != 0 )
        {
            int delta_scale = bs_read_se(b);
            nextScale = ( lastScale + delta_scale + 256 ) % 256;
            useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
        }
        scalingList[ j ] = ( nextScale == 0 ) ? lastScale : nextScale;
        lastScale = scalingList[ j ];
    }
}

//Appendix E.1.1 VUI parameters syntax
void read_vui_parameters(h264_stream_t* h, bs_t* b)
{
    sps_t* sps = h->sps;

    sps->vui.aspect_ratio_info_present_flag = bs_read_u1(b);
    if( sps->vui.aspect_ratio_info_present_flag )
    {
        sps->vui.aspect_ratio_idc = bs_read_u8(b);
        if( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            sps->vui.sar_width = bs_read_u(b,16);
            sps->vui.sar_height = bs_read_u(b,16);
        }
    }
    sps->vui.overscan_info_present_flag = bs_read_u1(b);
    if( sps->vui.overscan_info_present_flag )
    {
        sps->vui.overscan_appropriate_flag = bs_read_u1(b);
    }
    sps->vui.video_signal_type_present_flag = bs_read_u1(b);
    if( sps->vui.video_signal_type_present_flag )
    {
        sps->vui.video_format = bs_read_u(b,3);
        sps->vui.video_full_range_flag = bs_read_u1(b);
        sps->vui.colour_description_present_flag = bs_read_u1(b);
        if( sps->vui.colour_description_present_flag )
        {
            sps->vui.colour_primaries = bs_read_u8(b);
            sps->vui.transfer_characteristics = bs_read_u8(b);
            sps->vui.matrix_coefficients = bs_read_u8(b);
        }
    }
    sps->vui.chroma_loc_info_present_flag = bs_read_u1(b);
    if( sps->vui.chroma_loc_info_present_flag )
    {
        sps->vui.chroma_sample_loc_type_top_field = bs_read_ue(b);
        sps->vui.chroma_sample_loc_type_bottom_field = bs_read_ue(b);
    }
    sps->vui.timing_info_present_flag = bs_read_u1(b);
    if( sps->vui.timing_info_present_flag )
    {
        sps->vui.num_units_in_tick = bs_read_u(b,32);
        sps->vui.time_scale = bs_read_u(b,32);
        sps->vui.fixed_frame_rate_flag = bs_read_u1(b);
    }
    sps->vui.nal_hrd_parameters_present_flag = bs_read_u1(b);
    if( sps->vui.nal_hrd_parameters_present_flag )
    {
        read_hrd_parameters(h, b);
    }
    sps->vui.vcl_hrd_parameters_present_flag = bs_read_u1(b);
    if( sps->vui.vcl_hrd_parameters_present_flag )
    {
        read_hrd_parameters(h, b);
    }
    if( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        sps->vui.low_delay_hrd_flag = bs_read_u1(b);
    }
    sps->vui.pic_struct_present_flag = bs_read_u1(b);
    sps->vui.bitstream_restriction_flag = bs_read_u1(b);
    if( sps->vui.bitstream_restriction_flag )
    {
        sps->vui.motion_vectors_over_pic_boundaries_flag = bs_read_u1(b);
        sps->vui.max_bytes_per_pic_denom = bs_read_ue(b);
        sps->vui.max_bits_per_mb_denom = bs_read_ue(b);
        sps->vui.log2_max_mv_length_horizontal = bs_read_ue(b);
        sps->vui.log2_max_mv_length_vertical = bs_read_ue(b);
        sps->vui.num_reorder_frames = bs_read_ue(b);
        sps->vui.max_dec_frame_buffering = bs_read_ue(b);
    }
}


//Appendix E.1.2 HRD parameters syntax
void read_hrd_parameters(h264_stream_t* h, bs_t* b)
{
    sps_t* sps = h->sps;
    int SchedSelIdx;

    sps->hrd.cpb_cnt_minus1 = bs_read_ue(b);
    sps->hrd.bit_rate_scale = bs_read_u(b,4);
    sps->hrd.cpb_size_scale = bs_read_u(b,4);
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        sps->hrd.bit_rate_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        sps->hrd.cpb_size_value_minus1[ SchedSelIdx ] = bs_read_ue(b);
        sps->hrd.cbr_flag[ SchedSelIdx ] = bs_read_u1(b);
    }
    sps->hrd.initial_cpb_removal_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.cpb_removal_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.dpb_output_delay_length_minus1 = bs_read_u(b,5);
    sps->hrd.time_offset_length = bs_read_u(b,5);
}


/*
UNIMPLEMENTED
//7.3.2.1.2 Sequence parameter set extension RBSP syntax
int read_seq_parameter_set_extension_rbsp(bs_t* b, sps_ext_t* sps_ext) {
    seq_parameter_set_id = bs_read_ue(b);
    aux_format_idc = bs_read_ue(b);
    if( aux_format_idc != 0 ) {
        bit_depth_aux_minus8 = bs_read_ue(b);
        alpha_incr_flag = bs_read_u1(b);
        alpha_opaque_value = bs_read_u(v);
        alpha_transparent_value = bs_read_u(v);
    }
    additional_extension_flag = bs_read_u1(b);
    read_rbsp_trailing_bits();
}
*/

//7.3.2.2 Picture parameter set RBSP syntax
void read_pic_parameter_set_rbsp(h264_stream_t* h, bs_t* b)
{
    int pps_id = bs_read_ue(b);
    pps_t* pps = h->pps = h->pps_table[pps_id] ;

    memset(pps, 0, sizeof(pps_t));

    int i;
    int i_group;

    pps->pic_parameter_set_id = pps_id;
    pps->seq_parameter_set_id = bs_read_ue(b);
    pps->entropy_coding_mode_flag = bs_read_u1(b);
    pps->pic_order_present_flag = bs_read_u1(b);
    pps->num_slice_groups_minus1 = bs_read_ue(b);

    if( pps->num_slice_groups_minus1 > 0 )
    {
        pps->slice_group_map_type = bs_read_ue(b);
        if( pps->slice_group_map_type == 0 )
        {
            for( i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                pps->run_length_minus1[ i_group ] = bs_read_ue(b);
            }
        }
        else if( pps->slice_group_map_type == 2 )
        {
            for( i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                pps->top_left[ i_group ] = bs_read_ue(b);
                pps->bottom_right[ i_group ] = bs_read_ue(b);
            }
        }
        else if( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            pps->slice_group_change_direction_flag = bs_read_u1(b);
            pps->slice_group_change_rate_minus1 = bs_read_ue(b);
        }
        else if( pps->slice_group_map_type == 6 )
        {
            pps->pic_size_in_map_units_minus1 = bs_read_ue(b);
            for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                pps->slice_group_id[ i ] = bs_read_u(b, intlog2( pps->num_slice_groups_minus1 + 1 ) ); // was u(v)
            }
        }
    }
    pps->num_ref_idx_l0_active_minus1 = bs_read_ue(b);
    pps->num_ref_idx_l1_active_minus1 = bs_read_ue(b);
    pps->weighted_pred_flag = bs_read_u1(b);
    pps->weighted_bipred_idc = bs_read_u(b,2);
    pps->pic_init_qp_minus26 = bs_read_se(b);
    pps->pic_init_qs_minus26 = bs_read_se(b);
    pps->chroma_qp_index_offset = bs_read_se(b);
    pps->deblocking_filter_control_present_flag = bs_read_u1(b);
    pps->constrained_intra_pred_flag = bs_read_u1(b);
    pps->redundant_pic_cnt_present_flag = bs_read_u1(b);

    pps->_more_rbsp_data_present = more_rbsp_data(h, b);
    if( pps->_more_rbsp_data_present )
    {
        pps->transform_8x8_mode_flag = bs_read_u1(b);
        pps->pic_scaling_matrix_present_flag = bs_read_u1(b);
        if( pps->pic_scaling_matrix_present_flag )
        {
            for( i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                pps->pic_scaling_list_present_flag[ i ] = bs_read_u1(b);
                if( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        read_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                      pps->UseDefaultScalingMatrix4x4Flag[ i ] );
                    }
                    else
                    {
                        read_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                      pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
        pps->second_chroma_qp_index_offset = bs_read_se(b);
    }
    read_rbsp_trailing_bits(h, b);
}

//7.3.2.3 Supplemental enhancement information RBSP syntax
void read_sei_rbsp(h264_stream_t* h, bs_t* b)
{
    int i;
    for (i = 0; i < h->num_seis; i++)
    {
        sei_free(h->seis[i]);
    }
    
    h->num_seis = 0;
    do {
        h->num_seis++;
        h->seis = (sei_t**)realloc(h->seis, h->num_seis * sizeof(sei_t*));
        h->seis[h->num_seis - 1] = sei_new();
        h->sei = h->seis[h->num_seis - 1];
        read_sei_message(h, b);
    } while( more_rbsp_data(h, b) );
    read_rbsp_trailing_bits(h, b);
}

int _read_ff_coded_number(bs_t* b)
{
    int n1 = 0;
    int n2;
    do 
    {
        n2 = bs_read_u8(b);
        n1 += n2;
    } while (n2 == 0xff);
    return n1;
}

//7.3.2.3.1 Supplemental enhancement information message syntax
void read_sei_message(h264_stream_t* h, bs_t* b)
{
    h->sei->payloadType = _read_ff_coded_number(b);
    h->sei->payloadSize = _read_ff_coded_number(b);
    read_sei_payload( h, b, h->sei->payloadType, h->sei->payloadSize );
}

//7.3.2.4 Access unit delimiter RBSP syntax
void read_access_unit_delimiter_rbsp(h264_stream_t* h, bs_t* b)
{
    h->aud->primary_pic_type = bs_read_u(b,3);
    read_rbsp_trailing_bits(h, b);
}

//7.3.2.5 End of sequence RBSP syntax
void read_end_of_seq_rbsp(h264_stream_t* h, bs_t* b)
{
}

//7.3.2.6 End of stream RBSP syntax
void read_end_of_stream_rbsp(h264_stream_t* h, bs_t* b)
{
}

//7.3.2.7 Filler data RBSP syntax
void read_filler_data_rbsp(h264_stream_t* h, bs_t* b)
{
    while( bs_next_bits(b, 8) == 0xFF )
    {
        int ff_byte = bs_read_f(b,8);  // equal to 0xFF
    }
    read_rbsp_trailing_bits(h, b);
}

//7.3.2.8 Slice layer without partitioning RBSP syntax
void read_slice_layer_rbsp(h264_stream_t* h,  bs_t* b)
{
    read_slice_header(h, b);
    slice_data_rbsp_t* slice_data = h->slice_data;

    if ( slice_data != NULL )
    {
        if ( slice_data->rbsp_buf != NULL ) free( slice_data->rbsp_buf ); 
        uint8_t *sptr = b->p + (!!b->bits_left); // CABAC-specific: skip alignment bits, if there are any
        slice_data->rbsp_size = b->end - sptr;
        
        slice_data->rbsp_buf = (uint8_t*)malloc(slice_data->rbsp_size);
        memcpy( slice_data->rbsp_buf, sptr, slice_data->rbsp_size );
        // ugly hack: since next NALU starts at byte border, we are going to be padded by trailing_bits;
        return;
    }

    // FIXME should read or skip data
    //slice_data( ); /* all categories of slice_data( ) syntax */
    read_rbsp_slice_trailing_bits(h, b);
}

/*
// UNIMPLEMENTED
//7.3.2.9.1 Slice data partition A RBSP syntax
slice_data_partition_a_layer_rbsp( ) {
    read_slice_header( );             // only category 2
    slice_id = bs_read_ue(b)
    read_slice_data( );               // only category 2
    read_rbsp_slice_trailing_bits( ); // only category 2
}

//7.3.2.9.2 Slice data partition B RBSP syntax
slice_data_partition_b_layer_rbsp( ) {
    slice_id = bs_read_ue(b);    // only category 3
    if( redundant_pic_cnt_present_flag )
        redundant_pic_cnt = bs_read_ue(b);
    read_slice_data( );               // only category 3
    read_rbsp_slice_trailing_bits( ); // only category 3
}

//7.3.2.9.3 Slice data partition C RBSP syntax
slice_data_partition_c_layer_rbsp( ) {
    slice_id = bs_read_ue(b);    // only category 4
    if( redundant_pic_cnt_present_flag )
        redundant_pic_cnt = bs_read_ue(b);
    read_slice_data( );               // only category 4
    rbsp_slice_trailing_bits( ); // only category 4
}
*/

int
more_rbsp_trailing_data(h264_stream_t* h, bs_t* b) { return !bs_eof(b); }

//7.3.2.10 RBSP slice trailing bits syntax
void read_rbsp_slice_trailing_bits(h264_stream_t* h, bs_t* b)
{
    read_rbsp_trailing_bits(h, b);
    int cabac_zero_word;
    if( h->pps->entropy_coding_mode_flag )
    {
        while( more_rbsp_trailing_data(h, b) )
        {
            cabac_zero_word = bs_read_f(b,16); // equal to 0x0000
        }
    }
}

//7.3.2.11 RBSP trailing bits syntax
void read_rbsp_trailing_bits(h264_stream_t* h, bs_t* b)
{
    int rbsp_stop_one_bit = bs_read_u1( b ); // equal to 1

    while( !bs_byte_aligned(b) )
    {
        int rbsp_alignment_zero_bit = bs_read_u1( b ); // equal to 0
    }
}

//7.3.3 Slice header syntax
void read_slice_header(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    memset(sh, 0, sizeof(slice_header_t));

    sps_t* sps = NULL; // h->sps;
    pps_t* pps = NULL;//h->pps;
    nal_t* nal = h->nal;

    sh->first_mb_in_slice = bs_read_ue(b);
    sh->slice_type = bs_read_ue(b);
    sh->pic_parameter_set_id = bs_read_ue(b);

    pps = h->pps = h->pps_table[sh->pic_parameter_set_id];
    sps = h->sps = h->sps_table[pps->seq_parameter_set_id];

    sh->frame_num = bs_read_u(b, sps->log2_max_frame_num_minus4 + 4 ); // was u(v)
    if( !sps->frame_mbs_only_flag )
    {
        sh->field_pic_flag = bs_read_u1(b);
        if( sh->field_pic_flag )
        {
            sh->bottom_field_flag = bs_read_u1(b);
        }
    }
    if( nal->nal_unit_type == 5 )
    {
        sh->idr_pic_id = bs_read_ue(b);
    }
    if( sps->pic_order_cnt_type == 0 )
    {
        sh->pic_order_cnt_lsb = bs_read_u(b, sps->log2_max_pic_order_cnt_lsb_minus4 + 4 ); // was u(v)
        if( pps->pic_order_present_flag && !sh->field_pic_flag )
        {
            sh->delta_pic_order_cnt_bottom = bs_read_se(b);
        }
    }
    if( sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag )
    {
        sh->delta_pic_order_cnt[ 0 ] = bs_read_se(b);
        if( pps->pic_order_present_flag && !sh->field_pic_flag )
        {
            sh->delta_pic_order_cnt[ 1 ] = bs_read_se(b);
        }
    }
    if( pps->redundant_pic_cnt_present_flag )
    {
        sh->redundant_pic_cnt = bs_read_ue(b);
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        sh->direct_spatial_mv_pred_flag = bs_read_u1(b);
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_P ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        sh->num_ref_idx_active_override_flag = bs_read_u1(b);
        if( sh->num_ref_idx_active_override_flag )
        {
            sh->num_ref_idx_l0_active_minus1 = bs_read_ue(b); // FIXME does this modify the pps?
            if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
            {
                sh->num_ref_idx_l1_active_minus1 = bs_read_ue(b);
            }
        }
    }
    read_ref_pic_list_reordering(h, b);
    if( ( pps->weighted_pred_flag && ( is_slice_type( sh->slice_type, SH_SLICE_TYPE_P ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) ) ) ||
        ( pps->weighted_bipred_idc == 1 && is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) ) )
    {
        read_pred_weight_table(h, b);
    }
    if( nal->nal_ref_idc != 0 )
    {
        read_dec_ref_pic_marking(h, b);
    }
    if( pps->entropy_coding_mode_flag && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        sh->cabac_init_idc = bs_read_ue(b);
    }
    sh->slice_qp_delta = bs_read_se(b);
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) )
        {
            sh->sp_for_switch_flag = bs_read_u1(b);
        }
        sh->slice_qs_delta = bs_read_se(b);
    }
    if( pps->deblocking_filter_control_present_flag )
    {
        sh->disable_deblocking_filter_idc = bs_read_ue(b);
        if( sh->disable_deblocking_filter_idc != 1 )
        {
            sh->slice_alpha_c0_offset_div2 = bs_read_se(b);
            sh->slice_beta_offset_div2 = bs_read_se(b);
        }
    }
    if( pps->num_slice_groups_minus1 > 0 &&
        pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5)
    {
        sh->slice_group_change_cycle = 
            bs_read_u(b, intlog2( pps->pic_size_in_map_units_minus1 +  
                                  pps->slice_group_change_rate_minus1 + 1 ) ); // was u(v) // FIXME add 2?
    }
    // bs_print_state(b);
}

//7.3.3.1 Reference picture list reordering syntax
void read_ref_pic_list_reordering(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    // FIXME should be an array

    if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        sh->rplr.ref_pic_list_reordering_flag_l0 = bs_read_u1(b);
        if( sh->rplr.ref_pic_list_reordering_flag_l0 )
        {
            do
            {
                sh->rplr.reordering_of_pic_nums_idc = bs_read_ue(b);
                if( sh->rplr.reordering_of_pic_nums_idc == 0 ||
                    sh->rplr.reordering_of_pic_nums_idc == 1 )
                {
                    sh->rplr.abs_diff_pic_num_minus1 = bs_read_ue(b);
                }
                else if( sh->rplr.reordering_of_pic_nums_idc == 2 )
                {
                    sh->rplr.long_term_pic_num = bs_read_ue(b);
                }
            } while( sh->rplr.reordering_of_pic_nums_idc != 3 && ! bs_eof(b) );
        }
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        sh->rplr.ref_pic_list_reordering_flag_l1 = bs_read_u1(b);
        if( sh->rplr.ref_pic_list_reordering_flag_l1 )
        {
            do
            {
                sh->rplr.reordering_of_pic_nums_idc = bs_read_ue(b);
                if( sh->rplr.reordering_of_pic_nums_idc == 0 ||
                    sh->rplr.reordering_of_pic_nums_idc == 1 )
                {
                    sh->rplr.abs_diff_pic_num_minus1 = bs_read_ue(b);
                }
                else if( sh->rplr.reordering_of_pic_nums_idc == 2 )
                {
                    sh->rplr.long_term_pic_num = bs_read_ue(b);
                }
            } while( sh->rplr.reordering_of_pic_nums_idc != 3 && ! bs_eof(b) );
        }
    }
}

//7.3.3.2 Prediction weight table syntax
void read_pred_weight_table(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    sps_t* sps = h->sps;
    pps_t* pps = h->pps;

    int i, j;

    sh->pwt.luma_log2_weight_denom = bs_read_ue(b);
    if( sps->chroma_format_idc != 0 )
    {
        sh->pwt.chroma_log2_weight_denom = bs_read_ue(b);
    }
    for( i = 0; i <= pps->num_ref_idx_l0_active_minus1; i++ )
    {
        sh->pwt.luma_weight_l0_flag[i] = bs_read_u1(b);
        if( sh->pwt.luma_weight_l0_flag[i] )
        {
            sh->pwt.luma_weight_l0[ i ] = bs_read_se(b);
            sh->pwt.luma_offset_l0[ i ] = bs_read_se(b);
        }
        if ( sps->chroma_format_idc != 0 )
        {
            sh->pwt.chroma_weight_l0_flag[i] = bs_read_u1(b);
            if( sh->pwt.chroma_weight_l0_flag[i] )
            {
                for( j =0; j < 2; j++ )
                {
                    sh->pwt.chroma_weight_l0[ i ][ j ] = bs_read_se(b);
                    sh->pwt.chroma_offset_l0[ i ][ j ] = bs_read_se(b);
                }
            }
        }
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        for( i = 0; i <= pps->num_ref_idx_l1_active_minus1; i++ )
        {
            sh->pwt.luma_weight_l1_flag[i] = bs_read_u1(b);
            if( sh->pwt.luma_weight_l1_flag[i] )
            {
                sh->pwt.luma_weight_l1[ i ] = bs_read_se(b);
                sh->pwt.luma_offset_l1[ i ] = bs_read_se(b);
            }
            if( sps->chroma_format_idc != 0 )
            {
                sh->pwt.chroma_weight_l1_flag[i] = bs_read_u1(b);
                if( sh->pwt.chroma_weight_l1_flag[i] )
                {
                    for( j = 0; j < 2; j++ )
                    {
                        sh->pwt.chroma_weight_l1[ i ][ j ] = bs_read_se(b);
                        sh->pwt.chroma_offset_l1[ i ][ j ] = bs_read_se(b);
                    }
                }
            }
        }
    }
}

//7.3.3.3 Decoded reference picture marking syntax
void read_dec_ref_pic_marking(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    // FIXME should be an array

    if( h->nal->nal_unit_type == 5 )
    {
        sh->drpm.no_output_of_prior_pics_flag = bs_read_u1(b);
        sh->drpm.long_term_reference_flag = bs_read_u1(b);
    }
    else
    {
        sh->drpm.adaptive_ref_pic_marking_mode_flag = bs_read_u1(b);
        if( sh->drpm.adaptive_ref_pic_marking_mode_flag )
        {
            do
            {
                sh->drpm.memory_management_control_operation = bs_read_ue(b);
                if( sh->drpm.memory_management_control_operation == 1 ||
                    sh->drpm.memory_management_control_operation == 3 )
                {
                    sh->drpm.difference_of_pic_nums_minus1 = bs_read_ue(b);
                }
                if(sh->drpm.memory_management_control_operation == 2 )
                {
                    sh->drpm.long_term_pic_num = bs_read_ue(b);
                }
                if( sh->drpm.memory_management_control_operation == 3 ||
                    sh->drpm.memory_management_control_operation == 6 )
                {
                    sh->drpm.long_term_frame_idx = bs_read_ue(b);
                }
                if( sh->drpm.memory_management_control_operation == 4 )
                {
                    sh->drpm.max_long_term_frame_idx_plus1 = bs_read_ue(b);
                }
            } while( sh->drpm.memory_management_control_operation != 0 && ! bs_eof(b) );
        }
    }
}

/*
// UNIMPLEMENTED
//7.3.4 Slice data syntax
slice_data( )
{
    if( pps->entropy_coding_mode_flag )
        while( !byte_aligned( ) )
            cabac_alignment_one_bit = bs_read_f(1);
    CurrMbAddr = first_mb_in_slice * ( 1 + MbaffFrameFlag );
    moreDataFlag = 1;
    prevMbSkipped = 0;
    do {
        if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
            if( !pps->entropy_coding_mode_flag ) {
                mb_skip_run = bs_read_ue(b);
                prevMbSkipped = ( mb_skip_run > 0 );
                for( i=0; i<mb_skip_run; i++ )
                    CurrMbAddr = NextMbAddress( CurrMbAddr );
                moreDataFlag = more_rbsp_data( );
            } else {
                mb_skip_flag = bs_read_ae(v);
                moreDataFlag = !mb_skip_flag;
            }
        if( moreDataFlag ) {
            if( MbaffFrameFlag && ( CurrMbAddr % 2 == 0 ||
                                    ( CurrMbAddr % 2 == 1 && prevMbSkipped ) ) )
                mb_field_decoding_flag = bs_read_u1(b) | bs_read_ae(v);
            macroblock_layer( );
        }
        if( !pps->entropy_coding_mode_flag )
            moreDataFlag = more_rbsp_data( );
        else {
            if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
                prevMbSkipped = mb_skip_flag;
            if( MbaffFrameFlag && CurrMbAddr % 2 == 0 )
                moreDataFlag = 1;
            else {
                end_of_slice_flag = bs_read_ae(v);
                moreDataFlag = !end_of_slice_flag;
            }
        }
        CurrMbAddr = NextMbAddress( CurrMbAddr );
    } while( moreDataFlag );
}
*/


/***************************** writing ******************************/

#define DBG_START \
    bs_t* b2 = (bs_t*)malloc(sizeof(bs_t)); \
    bs_init(b2, b->p, b->end - b->p); \
    h264_stream_t* h2 = (h264_stream_t*)malloc(sizeof(h264_stream_t));\
    h2->sps=h->sps; h2->pps=h->pps; h2->nal=h->nal; h2->sh=h->sh;  \

#define DBG_END \
    free(h2); \
    free(b2); \

#define DBG \
  printf("line %d:",  __LINE__ ); \
  debug_bs(b); \
  b2->p = b2->start; b2->bits_left = 8; \
  /* read_slice_header(h2, b2); */\
  /* if (h2->sh->drpm.adaptive_ref_pic_marking_mode_flag) { printf(" X"); }; */ \
  printf("\n"); \

/**
 Write a NAL unit to a byte buffer.
 The NAL which is written out has a type determined by h->nal and data which comes from other fields within h depending on its type.
 @param[in,out]  h          the stream object
 @param[out]     buf        the buffer
 @param[in]      size       the size of the buffer
 @return                    the length of data actually written
 */
//7.3.1 NAL unit syntax
int write_nal_unit(h264_stream_t* h, uint8_t* buf, int size)
{
    nal_t* nal = h->nal;

    bs_t* b = bs_new(buf, size);

    bs_write_f(b,1, nal->forbidden_zero_bit);
    bs_write_u(b,2, nal->nal_ref_idc);
    bs_write_u(b,5, nal->nal_unit_type);

    bs_free(b);

    int rbsp_size = size*3/4; // NOTE this may have to be slightly smaller (3/4 smaller, worst case) in order to be guaranteed to fit
    uint8_t* rbsp_buf = (uint8_t*)calloc(1, rbsp_size); // FIXME can use malloc?
    int nal_size = size;

    b = bs_new(rbsp_buf, rbsp_size);

    switch ( nal->nal_unit_type )
    {
        case NAL_UNIT_TYPE_CODED_SLICE_IDR:
        case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:  
        case NAL_UNIT_TYPE_CODED_SLICE_AUX:
            write_slice_layer_rbsp(h, b);
            break;

        case NAL_UNIT_TYPE_SEI:
            write_sei_rbsp(h, b);
            break;

        case NAL_UNIT_TYPE_SPS: 
            write_seq_parameter_set_rbsp(h, b); 
            break;

        case NAL_UNIT_TYPE_PPS:   
            write_pic_parameter_set_rbsp(h, b);;
            break;

        case NAL_UNIT_TYPE_AUD:     
            write_access_unit_delimiter_rbsp(h, b); 
            break;

        case NAL_UNIT_TYPE_END_OF_SEQUENCE: 
            write_end_of_seq_rbsp(h, b);
            break;

        case NAL_UNIT_TYPE_END_OF_STREAM: 
            write_end_of_stream_rbsp(h, b);
            break;
//         case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A:
//         case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B:
//         case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C:
//         case NAL_UNIT_TYPE_FILLER:
//         case NAL_UNIT_TYPE_SPS_EXT:
//         case NAL_UNIT_TYPE_UNSPECIFIED:
        default:
            // here comes the reserved/unspecified/ignored stuff
            return 0;
    }


    if (bs_overrun(b)) { bs_free(b); free(rbsp_buf); return -1; }

    // now get the actual size used
    rbsp_size = bs_pos(b);

    int rc = rbsp_to_nal(rbsp_buf, &rbsp_size, buf, &nal_size);
    if (rc < 0) { bs_free(b); free(rbsp_buf); return -1; }

    bs_free(b);
    free(rbsp_buf);

    return nal_size;
}


//7.3.2.1 Sequence parameter set RBSP syntax
void write_seq_parameter_set_rbsp(h264_stream_t* h, bs_t* b)
{
    sps_t* sps = h->sps;

    int i;

    bs_write_u8(b, sps->profile_idc);
    bs_write_u1(b, sps->constraint_set0_flag);
    bs_write_u1(b, sps->constraint_set1_flag);
    bs_write_u1(b, sps->constraint_set2_flag);
    bs_write_u1(b, sps->constraint_set3_flag);
    bs_write_u1(b, sps->constraint_set4_flag);
    bs_write_u1(b, sps->constraint_set5_flag);
    bs_write_u(b,2, 0);  /* reserved_zero_2bits */
    bs_write_u8(b, sps->level_idc);
    bs_write_ue(b, sps->seq_parameter_set_id);
    if( sps->profile_idc == 100 || sps->profile_idc == 110 ||
        sps->profile_idc == 122 || sps->profile_idc == 144 )
    {
        bs_write_ue(b, sps->chroma_format_idc);
        if( sps->chroma_format_idc == 3 )
        {
            bs_write_u1(b, sps->residual_colour_transform_flag);
        }
        bs_write_ue(b, sps->bit_depth_luma_minus8);
        bs_write_ue(b, sps->bit_depth_chroma_minus8);
        bs_write_u1(b, sps->qpprime_y_zero_transform_bypass_flag);
        bs_write_u1(b, sps->seq_scaling_matrix_present_flag);
        if( sps->seq_scaling_matrix_present_flag )
        {
            for( i = 0; i < 8; i++ )
            {
                bs_write_u1(b, sps->seq_scaling_list_present_flag[ i ]);
                if( sps->seq_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        write_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                                      sps->UseDefaultScalingMatrix4x4Flag[ i ]);
                    }
                    else
                    {
                        write_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                                      sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
    }
    bs_write_ue(b, sps->log2_max_frame_num_minus4);
    bs_write_ue(b, sps->pic_order_cnt_type);
    if( sps->pic_order_cnt_type == 0 )
    {
        bs_write_ue(b, sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    else if( sps->pic_order_cnt_type == 1 )
    {
        bs_write_u1(b, sps->delta_pic_order_always_zero_flag);
        bs_write_se(b, sps->offset_for_non_ref_pic);
        bs_write_se(b, sps->offset_for_top_to_bottom_field);
        bs_write_ue(b, sps->num_ref_frames_in_pic_order_cnt_cycle);
        for( i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++ )
        {
            bs_write_se(b, sps->offset_for_ref_frame[ i ]);
        }
    }
    bs_write_ue(b, sps->num_ref_frames);
    bs_write_u1(b, sps->gaps_in_frame_num_value_allowed_flag);
    bs_write_ue(b, sps->pic_width_in_mbs_minus1);
    bs_write_ue(b, sps->pic_height_in_map_units_minus1);
    bs_write_u1(b, sps->frame_mbs_only_flag);
    if( !sps->frame_mbs_only_flag )
    {
        bs_write_u1(b, sps->mb_adaptive_frame_field_flag);
    }
    bs_write_u1(b, sps->direct_8x8_inference_flag);
    bs_write_u1(b, sps->frame_cropping_flag);
    if( sps->frame_cropping_flag )
    {
        bs_write_ue(b, sps->frame_crop_left_offset);
        bs_write_ue(b, sps->frame_crop_right_offset);
        bs_write_ue(b, sps->frame_crop_top_offset);
        bs_write_ue(b, sps->frame_crop_bottom_offset);
    }
    bs_write_u1(b, sps->vui_parameters_present_flag);
    if( sps->vui_parameters_present_flag )
    {
        write_vui_parameters(h, b);
    }
    write_rbsp_trailing_bits(h, b);
}

//7.3.2.1.1 Scaling list syntax
void write_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int useDefaultScalingMatrixFlag )
{
    int j;

    int lastScale = 8;
    int nextScale = 8;

    for( j = 0; j < sizeOfScalingList; j++ )
    {
        int delta_scale;

        if( nextScale != 0 )
        {
            // FIXME will not write in most compact way - could truncate list if all remaining elements are equal
            nextScale = scalingList[ j ];

            if (useDefaultScalingMatrixFlag)
            {
                nextScale = 0;
            }

            delta_scale = (nextScale - lastScale) % 256 ;
            bs_write_se(b, delta_scale);
        }

        lastScale = scalingList[ j ];
    }
}

//Appendix E.1.1 VUI parameters syntax
void
write_vui_parameters(h264_stream_t* h, bs_t* b)
{
    sps_t* sps = h->sps;

    bs_write_u1(b, sps->vui.aspect_ratio_info_present_flag);
    if( sps->vui.aspect_ratio_info_present_flag )
    {
        bs_write_u8(b, sps->vui.aspect_ratio_idc);
        if( sps->vui.aspect_ratio_idc == SAR_Extended )
        {
            bs_write_u(b,16, sps->vui.sar_width);
            bs_write_u(b,16, sps->vui.sar_height);
        }
    }
    bs_write_u1(b, sps->vui.overscan_info_present_flag);
    if( sps->vui.overscan_info_present_flag )
    {
        bs_write_u1(b, sps->vui.overscan_appropriate_flag);
    }
    bs_write_u1(b, sps->vui.video_signal_type_present_flag);
    if( sps->vui.video_signal_type_present_flag )
    {
        bs_write_u(b,3, sps->vui.video_format);
        bs_write_u1(b, sps->vui.video_full_range_flag);
        bs_write_u1(b, sps->vui.colour_description_present_flag);
        if( sps->vui.colour_description_present_flag )
        {
            bs_write_u8(b, sps->vui.colour_primaries);
            bs_write_u8(b, sps->vui.transfer_characteristics);
            bs_write_u8(b, sps->vui.matrix_coefficients);
        }
    }
    bs_write_u1(b, sps->vui.chroma_loc_info_present_flag);
    if( sps->vui.chroma_loc_info_present_flag )
    {
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_top_field);
        bs_write_ue(b, sps->vui.chroma_sample_loc_type_bottom_field);
    }
    bs_write_u1(b, sps->vui.timing_info_present_flag);
    if( sps->vui.timing_info_present_flag )
    {
        bs_write_u(b,32, sps->vui.num_units_in_tick);
        bs_write_u(b,32, sps->vui.time_scale);
        bs_write_u1(b, sps->vui.fixed_frame_rate_flag);
    }
    bs_write_u1(b, sps->vui.nal_hrd_parameters_present_flag);
    if( sps->vui.nal_hrd_parameters_present_flag )
    {
        write_hrd_parameters(h, b);
    }
    bs_write_u1(b, sps->vui.vcl_hrd_parameters_present_flag);
    if( sps->vui.vcl_hrd_parameters_present_flag )
    {
        write_hrd_parameters(h, b);
    }
    if( sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag )
    {
        bs_write_u1(b, sps->vui.low_delay_hrd_flag);
    }
    bs_write_u1(b, sps->vui.pic_struct_present_flag);
    bs_write_u1(b, sps->vui.bitstream_restriction_flag);
    if( sps->vui.bitstream_restriction_flag )
    {
        bs_write_u1(b, sps->vui.motion_vectors_over_pic_boundaries_flag);
        bs_write_ue(b, sps->vui.max_bytes_per_pic_denom);
        bs_write_ue(b, sps->vui.max_bits_per_mb_denom);
        bs_write_ue(b, sps->vui.log2_max_mv_length_horizontal);
        bs_write_ue(b, sps->vui.log2_max_mv_length_vertical);
        bs_write_ue(b, sps->vui.num_reorder_frames);
        bs_write_ue(b, sps->vui.max_dec_frame_buffering);
    }
}

//Appendix E.1.2 HRD parameters syntax
void
write_hrd_parameters(h264_stream_t* h, bs_t* b)
{
    sps_t* sps = h->sps;
    int SchedSelIdx;

    bs_write_ue(b, sps->hrd.cpb_cnt_minus1);
    bs_write_u(b,4, sps->hrd.bit_rate_scale);
    bs_write_u(b,4, sps->hrd.cpb_size_scale);
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        bs_write_ue(b, sps->hrd.bit_rate_value_minus1[ SchedSelIdx ]);
        bs_write_ue(b, sps->hrd.cpb_size_value_minus1[ SchedSelIdx ]);
        bs_write_u1(b, sps->hrd.cbr_flag[ SchedSelIdx ]);
    }
    bs_write_u(b,5, sps->hrd.initial_cpb_removal_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.cpb_removal_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.dpb_output_delay_length_minus1);
    bs_write_u(b,5, sps->hrd.time_offset_length);
}

/*
UNIMPLEMENTED
//7.3.2.1.2 Sequence parameter set extension RBSP syntax
int write_seq_parameter_set_extension_rbsp(bs_t* b, sps_ext_t* sps_ext) {
    bs_write_ue(b, seq_parameter_set_id);
    bs_write_ue(b, aux_format_idc);
    if( aux_format_idc != 0 ) {
        bs_write_ue(b, bit_depth_aux_minus8);
        bs_write_u1(b, alpha_incr_flag);
        bs_write_u(v, alpha_opaque_value);
        bs_write_u(v, alpha_transparent_value);
    }
    bs_write_u1(b, additional_extension_flag);
    write_rbsp_trailing_bits();
}
*/

//7.3.2.2 Picture parameter set RBSP syntax
void write_pic_parameter_set_rbsp(h264_stream_t* h, bs_t* b)
{
    pps_t* pps = h->pps;

    int i;
    int i_group;

    bs_write_ue(b, pps->pic_parameter_set_id);
    bs_write_ue(b, pps->seq_parameter_set_id);
    bs_write_u1(b, pps->entropy_coding_mode_flag);
    bs_write_u1(b, pps->pic_order_present_flag);
    bs_write_ue(b, pps->num_slice_groups_minus1);

    if( pps->num_slice_groups_minus1 > 0 )
    {
        bs_write_ue(b, pps->slice_group_map_type);
        if( pps->slice_group_map_type == 0 )
        {
            for( i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->run_length_minus1[ i_group ]);
            }
        }
        else if( pps->slice_group_map_type == 2 )
        {
            for( i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++ )
            {
                bs_write_ue(b, pps->top_left[ i_group ]);
                bs_write_ue(b, pps->bottom_right[ i_group ]);
            }
        }
        else if( pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5 )
        {
            bs_write_u1(b, pps->slice_group_change_direction_flag);
            bs_write_ue(b, pps->slice_group_change_rate_minus1);
        }
        else if( pps->slice_group_map_type == 6 )
        {
            bs_write_ue(b, pps->pic_size_in_map_units_minus1);
            for( i = 0; i <= pps->pic_size_in_map_units_minus1; i++ )
            {
                bs_write_u(b, intlog2( pps->num_slice_groups_minus1 + 1 ), pps->slice_group_id[ i ] ); // was u(v)
            }
        }
    }
    bs_write_ue(b, pps->num_ref_idx_l0_active_minus1);
    bs_write_ue(b, pps->num_ref_idx_l1_active_minus1);
    bs_write_u1(b, pps->weighted_pred_flag);
    bs_write_u(b,2, pps->weighted_bipred_idc);
    bs_write_se(b, pps->pic_init_qp_minus26);
    bs_write_se(b, pps->pic_init_qs_minus26);
    bs_write_se(b, pps->chroma_qp_index_offset);
    bs_write_u1(b, pps->deblocking_filter_control_present_flag);
    bs_write_u1(b, pps->constrained_intra_pred_flag);
    bs_write_u1(b, pps->redundant_pic_cnt_present_flag);
    
    if ( pps->_more_rbsp_data_present )
    {
        bs_write_u1(b, pps->transform_8x8_mode_flag);
        bs_write_u1(b, pps->pic_scaling_matrix_present_flag);
        if( pps->pic_scaling_matrix_present_flag )
        {
            for( i = 0; i < 6 + 2* pps->transform_8x8_mode_flag; i++ )
            {
                bs_write_u1(b, pps->pic_scaling_list_present_flag[ i ]);
                if( pps->pic_scaling_list_present_flag[ i ] )
                {
                    if( i < 6 )
                    {
                        write_scaling_list( b, pps->ScalingList4x4[ i ], 16,
                                      pps->UseDefaultScalingMatrix4x4Flag[ i ] );
                    }
                    else
                    {
                        write_scaling_list( b, pps->ScalingList8x8[ i - 6 ], 64,
                                      pps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                    }
                }
            }
        }
        bs_write_se(b, pps->second_chroma_qp_index_offset);
    }
    
    write_rbsp_trailing_bits(h, b);
}

//7.3.2.3 Supplemental enhancement information RBSP syntax
void write_sei_rbsp(h264_stream_t* h, bs_t* b)
{
    int i;
    for (i = 0; i < h->num_seis; i++)
    {
        h->sei = h->seis[i];
        write_sei_message(h, b);
    }
    //since hei is temp, let seis list free what needs to be freed
    //and leave sei null
    h->sei = NULL;
    write_rbsp_trailing_bits(h, b);
}

void _write_ff_coded_number(bs_t* b, int n)
{
    while (1)
    {
        if (n > 0xff)
        {
            bs_write_u8(b, 0xff);
            n -= 0xff;
        }
        else
        {
            bs_write_u8(b, n);
            break;
        }
    }
}

//7.3.2.3.1 Supplemental enhancement information message syntax
void write_sei_message(h264_stream_t* h, bs_t* b)
{
    // FIXME need some way to calculate size, or write message then write size
    _write_ff_coded_number(b, h->sei->payloadType);
    _write_ff_coded_number(b, h->sei->payloadSize);
    write_sei_payload( h, b, h->sei->payloadType, h->sei->payloadSize );
}

//7.3.2.4 Access unit delimiter RBSP syntax
void write_access_unit_delimiter_rbsp(h264_stream_t* h, bs_t* b)
{
    bs_write_u(b,3, h->aud->primary_pic_type);
    write_rbsp_trailing_bits(h, b);
}

//7.3.2.5 End of sequence RBSP syntax
void write_end_of_seq_rbsp(h264_stream_t* h, bs_t* b)
{
}

//7.3.2.6 End of stream RBSP syntax
void write_end_of_stream_rbsp(h264_stream_t* h, bs_t* b)
{
}

//7.3.2.7 Filler data RBSP syntax
void write_filler_data_rbsp(h264_stream_t* h, bs_t* b)
{
    int ff_byte = 0xFF; //FIXME
    while( bs_next_bits(b, 8) == 0xFF )
    {
        bs_write_f(b,8, ff_byte);  // equal to 0xFF
    }
    write_rbsp_trailing_bits(h, b);
}

//7.3.2.8 Slice layer without partitioning RBSP syntax
void write_slice_layer_rbsp(h264_stream_t* h, bs_t* b)
{
    write_slice_header(h, b);
    slice_data_rbsp_t* slice_data = h->slice_data;

    if ( slice_data != NULL )
    {

        if ( h->pps->entropy_coding_mode_flag )
        {
           // CABAC alignment bits
            while ( ! bs_byte_aligned(b) ) bs_write_u1(b, 0x01); 
        }

        bs_write_bytes( b, slice_data->rbsp_buf, slice_data->rbsp_size ); 
        return; // HACK slice trailing bits already included
    }
    //slice_data( ); /* all categories of slice_data( ) syntax */
    write_rbsp_slice_trailing_bits(h, b);
}

/*
// UNIMPLEMENTED
//7.3.2.9.1 Slice data partition A RBSP syntax
slice_data_partition_a_layer_rbsp( )
{
    write_slice_header( );             // only category 2
    bs_write_ue(b, slice_id)
    write_slice_data( );               // only category 2
    write_rbsp_slice_trailing_bits( ); // only category 2
}

//7.3.2.9.2 Slice data partition B RBSP syntax
slice_data_partition_b_layer_rbsp( )
{
    bs_write_ue(b, slice_id);    // only category 3
    if( redundant_pic_cnt_present_flag )
        bs_write_ue(b, redundant_pic_cnt);
    write_slice_data( );               // only category 3
    write_rbsp_slice_trailing_bits( ); // only category 3
}

//7.3.2.9.3 Slice data partition C RBSP syntax
slice_data_partition_c_layer_rbsp( )
{
    bs_write_ue(b, slice_id);    // only category 4
    if( redundant_pic_cnt_present_flag )
        bs_write_ue(b, redundant_pic_cnt);
    write_slice_data( );               // only category 4
    rbsp_slice_trailing_bits( ); // only category 4
}
*/

//7.3.2.10 RBSP slice trailing bits syntax
void write_rbsp_slice_trailing_bits(h264_stream_t* h, bs_t* b)
{
    write_rbsp_trailing_bits(h, b);

    if( h->pps->entropy_coding_mode_flag )
    {
        /*
        // 9.3.4.6 Byte stuffing process (informative)
        // NOTE do not write any cabac_zero_word for now - this appears to be optional
        while( more_rbsp_trailing_data(h, b) )
        {
            bs_write_f(b,16, 0x0000); // cabac_zero_word
        }
        */
    }
}

//7.3.2.11 RBSP trailing bits syntax
void write_rbsp_trailing_bits(h264_stream_t* h, bs_t* b)
{
    int rbsp_stop_one_bit = 1;
    int rbsp_alignment_zero_bit = 0;

    bs_write_f(b,1, rbsp_stop_one_bit); // equal to 1
    while( !bs_byte_aligned(b) )
    {
        bs_write_f(b,1, rbsp_alignment_zero_bit); // equal to 0
    }
}

//7.3.3 Slice header syntax
void write_slice_header(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;    
    pps_t* pps = h->pps_table[sh->pic_parameter_set_id];
    sps_t* sps = h->sps_table[pps->seq_parameter_set_id];
    nal_t* nal = h->nal;

    //DBG_START
    //h2->sh = (slice_header_t*)malloc(sizeof(slice_header_t));

    bs_write_ue(b, sh->first_mb_in_slice);
    bs_write_ue(b, sh->slice_type);
    bs_write_ue(b, sh->pic_parameter_set_id);
    bs_write_u(b, sps->log2_max_frame_num_minus4 + 4, sh->frame_num ); // was u(v)
    if( !sps->frame_mbs_only_flag )
    {
        bs_write_u1(b, sh->field_pic_flag);
        if( sh->field_pic_flag )
        {
            bs_write_u1(b, sh->bottom_field_flag);
        }
    }
    if( nal->nal_unit_type == 5 )
    {
        bs_write_ue(b, sh->idr_pic_id);
    }
    if( sps->pic_order_cnt_type == 0 )
    {
        bs_write_u(b, sps->log2_max_pic_order_cnt_lsb_minus4 + 4, sh->pic_order_cnt_lsb ); // was u(v)
        if( pps->pic_order_present_flag && !sh->field_pic_flag )
        {
            bs_write_se(b, sh->delta_pic_order_cnt_bottom);
        }
    }
    if( sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag )
    {
        bs_write_se(b, sh->delta_pic_order_cnt[ 0 ]);
        if( pps->pic_order_present_flag && !sh->field_pic_flag )
        {
            bs_write_se(b, sh->delta_pic_order_cnt[ 1 ]);
        }
    }
    if( pps->redundant_pic_cnt_present_flag )
    {
        bs_write_ue(b, sh->redundant_pic_cnt);
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        bs_write_u1(b, sh->direct_spatial_mv_pred_flag);
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_P ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        bs_write_u1(b, sh->num_ref_idx_active_override_flag);
        if( sh->num_ref_idx_active_override_flag )
        {
            bs_write_ue(b, sh->num_ref_idx_l0_active_minus1); // FIXME does this modify the pps?
            if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
            {
                bs_write_ue(b, sh->num_ref_idx_l1_active_minus1);
            }
        }
    }
    write_ref_pic_list_reordering(h, b);
    if( ( pps->weighted_pred_flag && ( is_slice_type( sh->slice_type, SH_SLICE_TYPE_P ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) ) ) ||
        ( pps->weighted_bipred_idc == 1 && is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) ) )
    {
        write_pred_weight_table(h, b);
    }
    if( nal->nal_ref_idc != 0 )
    {
        write_dec_ref_pic_marking(h, b);
    }
    if( pps->entropy_coding_mode_flag && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        bs_write_ue(b, sh->cabac_init_idc);
    }
    bs_write_se(b, sh->slice_qp_delta);
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) || is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_SP ) )
        {
            bs_write_u1(b, sh->sp_for_switch_flag);
        }
        bs_write_se(b, sh->slice_qs_delta);
    }
    if( pps->deblocking_filter_control_present_flag )
    {
        bs_write_ue(b, sh->disable_deblocking_filter_idc);
        if( sh->disable_deblocking_filter_idc != 1 )
        {
            bs_write_se(b, sh->slice_alpha_c0_offset_div2);
            bs_write_se(b, sh->slice_beta_offset_div2);
        }
    }
    if( pps->num_slice_groups_minus1 > 0 &&
        pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5)
    {
        bs_write_u(b, intlog2( pps->pic_size_in_map_units_minus1 +
                               pps->slice_group_change_rate_minus1 + 1 ),
                   sh->slice_group_change_cycle ); // was u(v) // FIXME add 2?
    }
    //bs_print_state(b);
    //free(h2->sh);
    //DBG_END
}

//7.3.3.1 Reference picture list reordering syntax
void write_ref_pic_list_reordering(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    // FIXME should be an array

    if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
    {
        bs_write_u1(b, sh->rplr.ref_pic_list_reordering_flag_l0);
        if( sh->rplr.ref_pic_list_reordering_flag_l0 )
        {
            do
            {
                bs_write_ue(b, sh->rplr.reordering_of_pic_nums_idc);
                if( sh->rplr.reordering_of_pic_nums_idc == 0 ||
                    sh->rplr.reordering_of_pic_nums_idc == 1 )
                {
                    bs_write_ue(b, sh->rplr.abs_diff_pic_num_minus1);
                }
                else if( sh->rplr.reordering_of_pic_nums_idc == 2 )
                {
                    bs_write_ue(b, sh->rplr.long_term_pic_num);
                }
            } while( sh->rplr.reordering_of_pic_nums_idc != 3 );
        }
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        bs_write_u1(b, sh->rplr.ref_pic_list_reordering_flag_l1);
        if( sh->rplr.ref_pic_list_reordering_flag_l1 )
        {
            do
            {
                bs_write_ue(b, sh->rplr.reordering_of_pic_nums_idc);
                if( sh->rplr.reordering_of_pic_nums_idc == 0 ||
                    sh->rplr.reordering_of_pic_nums_idc == 1 )
                {
                    bs_write_ue(b, sh->rplr.abs_diff_pic_num_minus1);
                }
                else if( sh->rplr.reordering_of_pic_nums_idc == 2 )
                {
                    bs_write_ue(b, sh->rplr.long_term_pic_num);
                }
            } while( sh->rplr.reordering_of_pic_nums_idc != 3 );
        }
    }
}

//7.3.3.2 Prediction weight table syntax
void write_pred_weight_table(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    sps_t* sps = h->sps;
    pps_t* pps = h->pps;

    int i, j;

    bs_write_ue(b, sh->pwt.luma_log2_weight_denom);
    if( sps->chroma_format_idc != 0 )
    {
        bs_write_ue(b, sh->pwt.chroma_log2_weight_denom);
    }
    for( i = 0; i <= pps->num_ref_idx_l0_active_minus1; i++ )
    {
        bs_write_u1(b, sh->pwt.luma_weight_l0_flag[i]);
        if( sh->pwt.luma_weight_l0_flag[i] )
        {
            bs_write_se(b, sh->pwt.luma_weight_l0[ i ]);
            bs_write_se(b, sh->pwt.luma_offset_l0[ i ]);
        }
        if ( sps->chroma_format_idc != 0 )
        {
            bs_write_u1(b, sh->pwt.chroma_weight_l0_flag[i]);
            if( sh->pwt.chroma_weight_l0_flag[i] )
            {
                for( j =0; j < 2; j++ )
                {
                    bs_write_se(b, sh->pwt.chroma_weight_l0[ i ][ j ]);
                    bs_write_se(b, sh->pwt.chroma_offset_l0[ i ][ j ]);
                }
            }
        }
    }
    if( is_slice_type( sh->slice_type, SH_SLICE_TYPE_B ) )
    {
        for( i = 0; i <= pps->num_ref_idx_l1_active_minus1; i++ )
        {
            bs_write_u1(b, sh->pwt.luma_weight_l1_flag[i]);
            if( sh->pwt.luma_weight_l1_flag[i] )
            {
                bs_write_se(b, sh->pwt.luma_weight_l1[ i ]);
                bs_write_se(b, sh->pwt.luma_offset_l1[ i ]);
            }
            if( sps->chroma_format_idc != 0 )
            {
                bs_write_u1(b, sh->pwt.chroma_weight_l1_flag[i]);
                if( sh->pwt.chroma_weight_l1_flag[i] )
                {
                    for( j = 0; j < 2; j++ )
                    {
                        bs_write_se(b, sh->pwt.chroma_weight_l1[ i ][ j ]);
                        bs_write_se(b, sh->pwt.chroma_offset_l1[ i ][ j ]);
                    }
                }
            }
        }
    }
}

//7.3.3.3 Decoded reference picture marking syntax
void write_dec_ref_pic_marking(h264_stream_t* h, bs_t* b)
{
    slice_header_t* sh = h->sh;
    // FIXME should be an array

    if( h->nal->nal_unit_type == 5 )
    {
        bs_write_u1(b, sh->drpm.no_output_of_prior_pics_flag);
        bs_write_u1(b, sh->drpm.long_term_reference_flag);
    }
    else
    {
        bs_write_u1(b, sh->drpm.adaptive_ref_pic_marking_mode_flag);
        if( sh->drpm.adaptive_ref_pic_marking_mode_flag )
        {
            do
            {
                bs_write_ue(b, sh->drpm.memory_management_control_operation);
                if( sh->drpm.memory_management_control_operation == 1 ||
                    sh->drpm.memory_management_control_operation == 3 )
                {
                    bs_write_ue(b, sh->drpm.difference_of_pic_nums_minus1);
                }
                if(sh->drpm.memory_management_control_operation == 2 )
                {
                    bs_write_ue(b, sh->drpm.long_term_pic_num);
                }
                if( sh->drpm.memory_management_control_operation == 3 ||
                    sh->drpm.memory_management_control_operation == 6 )
                {
                    bs_write_ue(b, sh->drpm.long_term_frame_idx);
                }
                if( sh->drpm.memory_management_control_operation == 4 )
                {
                    bs_write_ue(b, sh->drpm.max_long_term_frame_idx_plus1);
                }
            } while( sh->drpm.memory_management_control_operation != 0 );
        }
    }
}

/*
// UNIMPLEMENTED
//7.3.4 Slice data syntax
slice_data( )
{
    if( pps->entropy_coding_mode_flag )
        while( !byte_aligned( ) )
            bs_write_f(1, cabac_alignment_one_bit);
    CurrMbAddr = first_mb_in_slice * ( 1 + MbaffFrameFlag );
    moreDataFlag = 1;
    prevMbSkipped = 0;
    do {
        if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ssh->lice_type != SH_SLICE_TYPE_SI )
            if( !pps->entropy_coding_mode_flag ) {
                bs_write_ue(b, mb_skip_run);
                prevMbSkipped = ( mb_skip_run > 0 );
                for( i=0; i<mb_skip_run; i++ )
                    CurrMbAddr = NextMbAddress( CurrMbAddr );
                moreDataFlag = more_rbsp_data( );
            } else {
                bs_write_ae(v, mb_skip_flag);
                moreDataFlag = !mb_skip_flag;
            }
        if( moreDataFlag ) {
            if( MbaffFrameFlag && ( CurrMbAddr % 2 == 0 ||
                                    ( CurrMbAddr % 2 == 1 && prevMbSkipped ) ) )
                bs_write_u1(b) | bs_write_ae(v, mb_field_decoding_flag);
            macroblock_layer( );
        }
        if( !pps->entropy_coding_mode_flag )
            moreDataFlag = more_rbsp_data( );
        else {
            if( ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_I ) && ! is_slice_type( sh->slice_type, SH_SLICE_TYPE_SI ) )
                prevMbSkipped = mb_skip_flag;
            if( MbaffFrameFlag && CurrMbAddr % 2 == 0 )
                moreDataFlag = 1;
            else {
                bs_write_ae(v, end_of_slice_flag);
                moreDataFlag = !end_of_slice_flag;
            }
        }
        CurrMbAddr = NextMbAddress( CurrMbAddr );
    } while( moreDataFlag );
}
*/




/***************************** debug ******************************/

void debug_sps(sps_t* sps)
{
    printf("======= SPS =======\n");
    printf(" profile_idc : %d \n", sps->profile_idc );
    printf(" constraint_set0_flag : %d \n", sps->constraint_set0_flag );
    printf(" constraint_set1_flag : %d \n", sps->constraint_set1_flag );
    printf(" constraint_set2_flag : %d \n", sps->constraint_set2_flag );
    printf(" constraint_set3_flag : %d \n", sps->constraint_set3_flag );
    printf(" constraint_set4_flag : %d \n", sps->constraint_set4_flag );
    printf(" constraint_set5_flag : %d \n", sps->constraint_set5_flag );
    printf(" reserved_zero_2bits : %d \n", sps->reserved_zero_2bits );
    printf(" level_idc : %d \n", sps->level_idc );
    printf(" seq_parameter_set_id : %d \n", sps->seq_parameter_set_id );
    printf(" chroma_format_idc : %d \n", sps->chroma_format_idc );
    printf(" residual_colour_transform_flag : %d \n", sps->residual_colour_transform_flag );
    printf(" bit_depth_luma_minus8 : %d \n", sps->bit_depth_luma_minus8 );
    printf(" bit_depth_chroma_minus8 : %d \n", sps->bit_depth_chroma_minus8 );
    printf(" qpprime_y_zero_transform_bypass_flag : %d \n", sps->qpprime_y_zero_transform_bypass_flag );
    printf(" seq_scaling_matrix_present_flag : %d \n", sps->seq_scaling_matrix_present_flag );
    //  int seq_scaling_list_present_flag[8];
    //  void* ScalingList4x4[6];
    //  int UseDefaultScalingMatrix4x4Flag[6];
    //  void* ScalingList8x8[2];
    //  int UseDefaultScalingMatrix8x8Flag[2];
    printf(" log2_max_frame_num_minus4 : %d \n", sps->log2_max_frame_num_minus4 );
    printf(" pic_order_cnt_type : %d \n", sps->pic_order_cnt_type );
      printf("   log2_max_pic_order_cnt_lsb_minus4 : %d \n", sps->log2_max_pic_order_cnt_lsb_minus4 );
      printf("   delta_pic_order_always_zero_flag : %d \n", sps->delta_pic_order_always_zero_flag );
      printf("   offset_for_non_ref_pic : %d \n", sps->offset_for_non_ref_pic );
      printf("   offset_for_top_to_bottom_field : %d \n", sps->offset_for_top_to_bottom_field );
      printf("   num_ref_frames_in_pic_order_cnt_cycle : %d \n", sps->num_ref_frames_in_pic_order_cnt_cycle );
    //  int offset_for_ref_frame[256];
    printf(" num_ref_frames : %d \n", sps->num_ref_frames );
    printf(" gaps_in_frame_num_value_allowed_flag : %d \n", sps->gaps_in_frame_num_value_allowed_flag );
    printf(" pic_width_in_mbs_minus1 : %d \n", sps->pic_width_in_mbs_minus1 );
    printf(" pic_height_in_map_units_minus1 : %d \n", sps->pic_height_in_map_units_minus1 );
    printf(" frame_mbs_only_flag : %d \n", sps->frame_mbs_only_flag );
    printf(" mb_adaptive_frame_field_flag : %d \n", sps->mb_adaptive_frame_field_flag );
    printf(" direct_8x8_inference_flag : %d \n", sps->direct_8x8_inference_flag );
    printf(" frame_cropping_flag : %d \n", sps->frame_cropping_flag );
      printf("   frame_crop_left_offset : %d \n", sps->frame_crop_left_offset );
      printf("   frame_crop_right_offset : %d \n", sps->frame_crop_right_offset );
      printf("   frame_crop_top_offset : %d \n", sps->frame_crop_top_offset );
      printf("   frame_crop_bottom_offset : %d \n", sps->frame_crop_bottom_offset );
    printf(" vui_parameters_present_flag : %d \n", sps->vui_parameters_present_flag );

    printf("=== VUI ===\n");
    printf(" aspect_ratio_info_present_flag : %d \n", sps->vui.aspect_ratio_info_present_flag );
      printf("   aspect_ratio_idc : %d \n", sps->vui.aspect_ratio_idc );
        printf("     sar_width : %d \n", sps->vui.sar_width );
        printf("     sar_height : %d \n", sps->vui.sar_height );
    printf(" overscan_info_present_flag : %d \n", sps->vui.overscan_info_present_flag );
      printf("   overscan_appropriate_flag : %d \n", sps->vui.overscan_appropriate_flag );
    printf(" video_signal_type_present_flag : %d \n", sps->vui.video_signal_type_present_flag );
      printf("   video_format : %d \n", sps->vui.video_format );
      printf("   video_full_range_flag : %d \n", sps->vui.video_full_range_flag );
      printf("   colour_description_present_flag : %d \n", sps->vui.colour_description_present_flag );
        printf("     colour_primaries : %d \n", sps->vui.colour_primaries );
        printf("   transfer_characteristics : %d \n", sps->vui.transfer_characteristics );
        printf("   matrix_coefficients : %d \n", sps->vui.matrix_coefficients );
    printf(" chroma_loc_info_present_flag : %d \n", sps->vui.chroma_loc_info_present_flag );
      printf("   chroma_sample_loc_type_top_field : %d \n", sps->vui.chroma_sample_loc_type_top_field );
      printf("   chroma_sample_loc_type_bottom_field : %d \n", sps->vui.chroma_sample_loc_type_bottom_field );
    printf(" timing_info_present_flag : %d \n", sps->vui.timing_info_present_flag );
      printf("   num_units_in_tick : %d \n", sps->vui.num_units_in_tick );
      printf("   time_scale : %d \n", sps->vui.time_scale );
      printf("   fixed_frame_rate_flag : %d \n", sps->vui.fixed_frame_rate_flag );
    printf(" nal_hrd_parameters_present_flag : %d \n", sps->vui.nal_hrd_parameters_present_flag );
    printf(" vcl_hrd_parameters_present_flag : %d \n", sps->vui.vcl_hrd_parameters_present_flag );
      printf("   low_delay_hrd_flag : %d \n", sps->vui.low_delay_hrd_flag );
    printf(" pic_struct_present_flag : %d \n", sps->vui.pic_struct_present_flag );
    printf(" bitstream_restriction_flag : %d \n", sps->vui.bitstream_restriction_flag );
      printf("   motion_vectors_over_pic_boundaries_flag : %d \n", sps->vui.motion_vectors_over_pic_boundaries_flag );
      printf("   max_bytes_per_pic_denom : %d \n", sps->vui.max_bytes_per_pic_denom );
      printf("   max_bits_per_mb_denom : %d \n", sps->vui.max_bits_per_mb_denom );
      printf("   log2_max_mv_length_horizontal : %d \n", sps->vui.log2_max_mv_length_horizontal );
      printf("   log2_max_mv_length_vertical : %d \n", sps->vui.log2_max_mv_length_vertical );
      printf("   num_reorder_frames : %d \n", sps->vui.num_reorder_frames );
      printf("   max_dec_frame_buffering : %d \n", sps->vui.max_dec_frame_buffering );

    printf("=== HRD ===\n");
    printf(" cpb_cnt_minus1 : %d \n", sps->hrd.cpb_cnt_minus1 );
    printf(" bit_rate_scale : %d \n", sps->hrd.bit_rate_scale );
    printf(" cpb_size_scale : %d \n", sps->hrd.cpb_size_scale );
    int SchedSelIdx;
    for( SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++ )
    {
        printf("   bit_rate_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.bit_rate_value_minus1[SchedSelIdx] ); // up to cpb_cnt_minus1, which is <= 31
        printf("   cpb_size_value_minus1[%d] : %d \n", SchedSelIdx, sps->hrd.cpb_size_value_minus1[SchedSelIdx] );
        printf("   cbr_flag[%d] : %d \n", SchedSelIdx, sps->hrd.cbr_flag[SchedSelIdx] );
    }
    printf(" initial_cpb_removal_delay_length_minus1 : %d \n", sps->hrd.initial_cpb_removal_delay_length_minus1 );
    printf(" cpb_removal_delay_length_minus1 : %d \n", sps->hrd.cpb_removal_delay_length_minus1 );
    printf(" dpb_output_delay_length_minus1 : %d \n", sps->hrd.dpb_output_delay_length_minus1 );
    printf(" time_offset_length : %d \n", sps->hrd.time_offset_length );
}


void debug_pps(pps_t* pps)
{
    printf("======= PPS =======\n");
    printf(" pic_parameter_set_id : %d \n", pps->pic_parameter_set_id );
    printf(" seq_parameter_set_id : %d \n", pps->seq_parameter_set_id );
    printf(" entropy_coding_mode_flag : %d \n", pps->entropy_coding_mode_flag );
    printf(" pic_order_present_flag : %d \n", pps->pic_order_present_flag );
    printf(" num_slice_groups_minus1 : %d \n", pps->num_slice_groups_minus1 );
    printf(" slice_group_map_type : %d \n", pps->slice_group_map_type );
    //  int run_length_minus1[8]; // up to num_slice_groups_minus1, which is <= 7 in Baseline and Extended, 0 otheriwse
    //  int top_left[8];
    //  int bottom_right[8];
    //  int slice_group_change_direction_flag;
    //  int slice_group_change_rate_minus1;
    //  int pic_size_in_map_units_minus1;
    //  int slice_group_id[256]; // FIXME what size?
    printf(" num_ref_idx_l0_active_minus1 : %d \n", pps->num_ref_idx_l0_active_minus1 );
    printf(" num_ref_idx_l1_active_minus1 : %d \n", pps->num_ref_idx_l1_active_minus1 );
    printf(" weighted_pred_flag : %d \n", pps->weighted_pred_flag );
    printf(" weighted_bipred_idc : %d \n", pps->weighted_bipred_idc );
    printf(" pic_init_qp_minus26 : %d \n", pps->pic_init_qp_minus26 );
    printf(" pic_init_qs_minus26 : %d \n", pps->pic_init_qs_minus26 );
    printf(" chroma_qp_index_offset : %d \n", pps->chroma_qp_index_offset );
    printf(" deblocking_filter_control_present_flag : %d \n", pps->deblocking_filter_control_present_flag );
    printf(" constrained_intra_pred_flag : %d \n", pps->constrained_intra_pred_flag );
    printf(" redundant_pic_cnt_present_flag : %d \n", pps->redundant_pic_cnt_present_flag );
    printf(" transform_8x8_mode_flag : %d \n", pps->transform_8x8_mode_flag );
    printf(" pic_scaling_matrix_present_flag : %d \n", pps->pic_scaling_matrix_present_flag );
    //  int pic_scaling_list_present_flag[8];
    //  void* ScalingList4x4[6];
    //  int UseDefaultScalingMatrix4x4Flag[6];
    //  void* ScalingList8x8[2];
    //  int UseDefaultScalingMatrix8x8Flag[2];
    printf(" second_chroma_qp_index_offset : %d \n", pps->second_chroma_qp_index_offset );
}

void debug_slice_header(slice_header_t* sh)
{
    printf("======= Slice Header =======\n");
    printf(" first_mb_in_slice : %d \n", sh->first_mb_in_slice );
    const char* slice_type_name;
    switch(sh->slice_type)
    {
    case SH_SLICE_TYPE_P :       slice_type_name = "P slice"; break;
    case SH_SLICE_TYPE_B :       slice_type_name = "B slice"; break;
    case SH_SLICE_TYPE_I :       slice_type_name = "I slice"; break;
    case SH_SLICE_TYPE_SP :      slice_type_name = "SP slice"; break;
    case SH_SLICE_TYPE_SI :      slice_type_name = "SI slice"; break;
    case SH_SLICE_TYPE_P_ONLY :  slice_type_name = "P slice only"; break;
    case SH_SLICE_TYPE_B_ONLY :  slice_type_name = "B slice only"; break;
    case SH_SLICE_TYPE_I_ONLY :  slice_type_name = "I slice only"; break;
    case SH_SLICE_TYPE_SP_ONLY : slice_type_name = "SP slice only"; break;
    case SH_SLICE_TYPE_SI_ONLY : slice_type_name = "SI slice only"; break;
    default :                    slice_type_name = "Unknown"; break;
    }
    printf(" slice_type : %d ( %s ) \n", sh->slice_type, slice_type_name );

    printf(" pic_parameter_set_id : %d \n", sh->pic_parameter_set_id );
    printf(" frame_num : %d \n", sh->frame_num );
    printf(" field_pic_flag : %d \n", sh->field_pic_flag );
      printf(" bottom_field_flag : %d \n", sh->bottom_field_flag );
    printf(" idr_pic_id : %d \n", sh->idr_pic_id );
    printf(" pic_order_cnt_lsb : %d \n", sh->pic_order_cnt_lsb );
    printf(" delta_pic_order_cnt_bottom : %d \n", sh->delta_pic_order_cnt_bottom );
    // int delta_pic_order_cnt[ 2 ];
    printf(" redundant_pic_cnt : %d \n", sh->redundant_pic_cnt );
    printf(" direct_spatial_mv_pred_flag : %d \n", sh->direct_spatial_mv_pred_flag );
    printf(" num_ref_idx_active_override_flag : %d \n", sh->num_ref_idx_active_override_flag );
    printf(" num_ref_idx_l0_active_minus1 : %d \n", sh->num_ref_idx_l0_active_minus1 );
    printf(" num_ref_idx_l1_active_minus1 : %d \n", sh->num_ref_idx_l1_active_minus1 );
    printf(" cabac_init_idc : %d \n", sh->cabac_init_idc );
    printf(" slice_qp_delta : %d \n", sh->slice_qp_delta );
    printf(" sp_for_switch_flag : %d \n", sh->sp_for_switch_flag );
    printf(" slice_qs_delta : %d \n", sh->slice_qs_delta );
    printf(" disable_deblocking_filter_idc : %d \n", sh->disable_deblocking_filter_idc );
    printf(" slice_alpha_c0_offset_div2 : %d \n", sh->slice_alpha_c0_offset_div2 );
    printf(" slice_beta_offset_div2 : %d \n", sh->slice_beta_offset_div2 );
    printf(" slice_group_change_cycle : %d \n", sh->slice_group_change_cycle );

    printf("=== Prediction Weight Table ===\n");
        printf(" luma_log2_weight_denom : %d \n", sh->pwt.luma_log2_weight_denom );
        printf(" chroma_log2_weight_denom : %d \n", sh->pwt.chroma_log2_weight_denom );
     //   printf(" luma_weight_l0_flag : %d \n", sh->pwt.luma_weight_l0_flag );
        // int luma_weight_l0[64];
        // int luma_offset_l0[64];
    //    printf(" chroma_weight_l0_flag : %d \n", sh->pwt.chroma_weight_l0_flag );
        // int chroma_weight_l0[64][2];
        // int chroma_offset_l0[64][2];
     //   printf(" luma_weight_l1_flag : %d \n", sh->pwt.luma_weight_l1_flag );
        // int luma_weight_l1[64];
        // int luma_offset_l1[64];
    //    printf(" chroma_weight_l1_flag : %d \n", sh->pwt.chroma_weight_l1_flag );
        // int chroma_weight_l1[64][2];
        // int chroma_offset_l1[64][2];

    printf("=== Ref Pic List Reordering ===\n");
        printf(" ref_pic_list_reordering_flag_l0 : %d \n", sh->rplr.ref_pic_list_reordering_flag_l0 );
        printf(" ref_pic_list_reordering_flag_l1 : %d \n", sh->rplr.ref_pic_list_reordering_flag_l1 );
        // int reordering_of_pic_nums_idc;
        // int abs_diff_pic_num_minus1;
        // int long_term_pic_num;

    printf("=== Decoded Ref Pic Marking ===\n");
        printf(" no_output_of_prior_pics_flag : %d \n", sh->drpm.no_output_of_prior_pics_flag );
        printf(" long_term_reference_flag : %d \n", sh->drpm.long_term_reference_flag );
        printf(" adaptive_ref_pic_marking_mode_flag : %d \n", sh->drpm.adaptive_ref_pic_marking_mode_flag );
        // int memory_management_control_operation;
        // int difference_of_pic_nums_minus1;
        // int long_term_pic_num;
        // int long_term_frame_idx;
        // int max_long_term_frame_idx_plus1;

}

void debug_aud(aud_t* aud)
{
    printf("======= Access Unit Delimiter =======\n");
    const char* primary_pic_type_name;
    switch (aud->primary_pic_type)
    {
    case AUD_PRIMARY_PIC_TYPE_I :       primary_pic_type_name = "I"; break;
    case AUD_PRIMARY_PIC_TYPE_IP :      primary_pic_type_name = "I, P"; break;
    case AUD_PRIMARY_PIC_TYPE_IPB :     primary_pic_type_name = "I, P, B"; break;
    case AUD_PRIMARY_PIC_TYPE_SI :      primary_pic_type_name = "SI"; break;
    case AUD_PRIMARY_PIC_TYPE_SISP :    primary_pic_type_name = "SI, SP"; break;
    case AUD_PRIMARY_PIC_TYPE_ISI :     primary_pic_type_name = "I, SI"; break;
    case AUD_PRIMARY_PIC_TYPE_ISIPSP :  primary_pic_type_name = "I, SI, P, SP"; break;
    case AUD_PRIMARY_PIC_TYPE_ISIPSPB : primary_pic_type_name = "I, SI, P, SP, B"; break;
    default : primary_pic_type_name = "Unknown"; break;
    }
    printf(" primary_pic_type : %d ( %s ) \n", aud->primary_pic_type, primary_pic_type_name );
}

void debug_seis( h264_stream_t* h)
{
    sei_t** seis = h->seis;
    int num_seis = h->num_seis;

    printf("======= SEI =======\n");
    const char* sei_type_name;
    int i;
    for (i = 0; i < num_seis; i++)
    {
        sei_t* s = seis[i];
        switch(s->payloadType)
        {
        case SEI_TYPE_BUFFERING_PERIOD :          sei_type_name = "Buffering period"; break;
        case SEI_TYPE_PIC_TIMING :                sei_type_name = "Pic timing"; break;
        case SEI_TYPE_PAN_SCAN_RECT :             sei_type_name = "Pan scan rect"; break;
        case SEI_TYPE_FILLER_PAYLOAD :            sei_type_name = "Filler payload"; break;
        case SEI_TYPE_USER_DATA_REGISTERED_ITU_T_T35 : sei_type_name = "User data registered ITU-T T35"; break;
        case SEI_TYPE_USER_DATA_UNREGISTERED :    sei_type_name = "User data unregistered"; break;
        case SEI_TYPE_RECOVERY_POINT :            sei_type_name = "Recovery point"; break;
        case SEI_TYPE_DEC_REF_PIC_MARKING_REPETITION : sei_type_name = "Dec ref pic marking repetition"; break;
        case SEI_TYPE_SPARE_PIC :                 sei_type_name = "Spare pic"; break;
        case SEI_TYPE_SCENE_INFO :                sei_type_name = "Scene info"; break;
        case SEI_TYPE_SUB_SEQ_INFO :              sei_type_name = "Sub seq info"; break;
        case SEI_TYPE_SUB_SEQ_LAYER_CHARACTERISTICS : sei_type_name = "Sub seq layer characteristics"; break;
        case SEI_TYPE_SUB_SEQ_CHARACTERISTICS :   sei_type_name = "Sub seq characteristics"; break;
        case SEI_TYPE_FULL_FRAME_FREEZE :         sei_type_name = "Full frame freeze"; break;
        case SEI_TYPE_FULL_FRAME_FREEZE_RELEASE : sei_type_name = "Full frame freeze release"; break;
        case SEI_TYPE_FULL_FRAME_SNAPSHOT :       sei_type_name = "Full frame snapshot"; break;
        case SEI_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_START : sei_type_name = "Progressive refinement segment start"; break;
        case SEI_TYPE_PROGRESSIVE_REFINEMENT_SEGMENT_END : sei_type_name = "Progressive refinement segment end"; break;
        case SEI_TYPE_MOTION_CONSTRAINED_SLICE_GROUP_SET : sei_type_name = "Motion constrained slice group set"; break;
        case SEI_TYPE_FILM_GRAIN_CHARACTERISTICS : sei_type_name = "Film grain characteristics"; break;
        case SEI_TYPE_DEBLOCKING_FILTER_DISPLAY_PREFERENCE : sei_type_name = "Deblocking filter display preference"; break;
        case SEI_TYPE_STEREO_VIDEO_INFO :         sei_type_name = "Stereo video info"; break;
        default: sei_type_name = "Unknown"; break;
        }
        printf("=== %s ===\n", sei_type_name);
        printf(" payloadType : %d \n", s->payloadType );
        printf(" payloadSize : %d \n", s->payloadSize );

        printf(" payload : " );
        debug_bytes(s->payload, s->payloadSize);
    }
}

/**
 Print the contents of a NAL unit to standard output.
 The NAL which is printed out has a type determined by nal and data which comes from other fields within h depending on its type.
 @param[in]      h          the stream object
 @param[in]      nal        the nal unit
 */
void debug_nal(h264_stream_t* h, nal_t* nal)
{
    printf("==================== NAL ====================\n");
    printf(" forbidden_zero_bit : %d \n", nal->forbidden_zero_bit );
    printf(" nal_ref_idc : %d \n", nal->nal_ref_idc );
    // TODO make into subroutine
    const char* nal_unit_type_name;
    switch (nal->nal_unit_type)
    {
    case  NAL_UNIT_TYPE_UNSPECIFIED :                   nal_unit_type_name = "Unspecified"; break;
    case  NAL_UNIT_TYPE_CODED_SLICE_NON_IDR :           nal_unit_type_name = "Coded slice of a non-IDR picture"; break;
    case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A :  nal_unit_type_name = "Coded slice data partition A"; break;
    case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B :  nal_unit_type_name = "Coded slice data partition B"; break;
    case  NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C :  nal_unit_type_name = "Coded slice data partition C"; break;
    case  NAL_UNIT_TYPE_CODED_SLICE_IDR :               nal_unit_type_name = "Coded slice of an IDR picture"; break;
    case  NAL_UNIT_TYPE_SEI :                           nal_unit_type_name = "Supplemental enhancement information (SEI)"; break;
    case  NAL_UNIT_TYPE_SPS :                           nal_unit_type_name = "Sequence parameter set"; break;
    case  NAL_UNIT_TYPE_PPS :                           nal_unit_type_name = "Picture parameter set"; break;
    case  NAL_UNIT_TYPE_AUD :                           nal_unit_type_name = "Access unit delimiter"; break;
    case  NAL_UNIT_TYPE_END_OF_SEQUENCE :               nal_unit_type_name = "End of sequence"; break;
    case  NAL_UNIT_TYPE_END_OF_STREAM :                 nal_unit_type_name = "End of stream"; break;
    case  NAL_UNIT_TYPE_FILLER :                        nal_unit_type_name = "Filler data"; break;
    case  NAL_UNIT_TYPE_SPS_EXT :                       nal_unit_type_name = "Sequence parameter set extension"; break;
        // 14..18    // Reserved
    case  NAL_UNIT_TYPE_CODED_SLICE_AUX :               nal_unit_type_name = "Coded slice of an auxiliary coded picture without partitioning"; break;
        // 20..23    // Reserved
        // 24..31    // Unspecified
    default :                                           nal_unit_type_name = "Unknown"; break;
    }
    printf(" nal_unit_type : %d ( %s ) \n", nal->nal_unit_type, nal_unit_type_name );

    if( nal->nal_unit_type == NAL_UNIT_TYPE_CODED_SLICE_NON_IDR) { debug_slice_header(h->sh); }
    else if( nal->nal_unit_type == NAL_UNIT_TYPE_CODED_SLICE_IDR) { debug_slice_header(h->sh); }
    else if( nal->nal_unit_type == NAL_UNIT_TYPE_SPS) { debug_sps(h->sps); }
    else if( nal->nal_unit_type == NAL_UNIT_TYPE_PPS) { debug_pps(h->pps); }
    else if( nal->nal_unit_type == NAL_UNIT_TYPE_AUD) { debug_aud(h->aud); }
    else if( nal->nal_unit_type == NAL_UNIT_TYPE_SEI) { debug_seis( h ); }
}

void debug_bytes(uint8_t* buf, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
        if ((i+1) % 16 == 0) { printf ("\n"); }
    }
    printf("\n");
}
