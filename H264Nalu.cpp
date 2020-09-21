
#include "H264Nalu.h"
#include "stdio.h"
#include "assert.h"


H264Nalu::H264Nalu(/* args */)
{
}

H264Nalu::~H264Nalu()
{
}

int H264Nalu::naluToRbsp() 
{
    int nalu_size = len;
    int j = 0;
    int count = 0;

    // 遇到0x000003则把03去掉，包含以cabac_zero_word结尾时，尾部为0x000003的情况
    for (int i = 0; i < nalu_size; i++)
    {

        if (count == 2 && buf[i] == 0x03)
        {
            if (i == nalu_size - 1) // 结尾为0x000003
            {
                break; // 跳出循环
            }
            else
            {
                i++; // 继续下一个
                count = 0;
            }
        }

        buf[j] = buf[i];

        if (buf[i] == 0x00)
        {
            count++;
        }
        else
        {
            count = 0;
        }

        j++;
    }

    return j;
}

int H264Nalu::rbspToSodb() 
{
      int ctr_bit, bitoffset, last_byte_pos;
    bitoffset = 0;
    last_byte_pos = len - 1;

    // 0.从nalu->buf的最末尾的比特开始寻找
    ctr_bit = (buf[last_byte_pos] & (0x01 << bitoffset));

    // 1.循环找到trailing_bits中的rbsp_stop_one_bit
    while (ctr_bit == 0)
    {
        bitoffset++;
        if (bitoffset == 8)
        {
            // 因nalu->buf中保存的是nalu_header+RBSP，因此找到最后1字节的nalu_header就宣告RBSP查找结束
            if (last_byte_pos == 1)
                printf(" Panic: All zero data sequence in RBSP \n");
            assert(last_byte_pos != 1);
            last_byte_pos -= 1;
            bitoffset = 0;
        }
        ctr_bit = buf[last_byte_pos - 1] & (0x01 << bitoffset);
    }
    // 【注】函数开始已对last_byte_pos做减1处理，此时last_byte_pos表示相对于SODB的位置，然后赋值给nalu->len得到最终SODB的大小
    return last_byte_pos;
}




bool H264Nalu::isEnd()
{
    if (p >= end)
    {
        return true;
    }
    return false;
}

uint8_t H264Nalu::peekOneBit()
{
    uint8_t r = 0;

    if (!isEnd())
    {
        r = ((*(p)) >> (bits_left - 1)) & 0x01;
    }
    return r;
}

uint8_t H264Nalu::readOneBit()
{
    uint8_t r = 0; // 读取比特返回值

    // 1.剩余比特先减1
    bits_left--;

    if (!isEnd())
    {
        // 2.计算返回值
        r = ((*(p)) >> bits_left) & 0x01;
    }

    // 3.判断是否读到字节末尾，如果是指针位置移向下一字节，比特位初始为8
    if (bits_left == 0)
    {
        p++;
        bits_left = 8;
    }

    return r;
}

int H264Nalu::readBits(int n)
{
    int r = 0; // 读取比特返回值
    int i;     // 当前读取到的比特位索引
    for (i = 0; i < n; i++)
    {
        // 1.每次读取1比特，并依次从高位到低位放在r中
        r |= (readOneBit() << (n - i - 1));
    }
    return r;
}

int H264Nalu::readUe()
{
    int r = 0; // 解码得到的返回值
    int i = 0; // leadingZeroBits

    // 1.计算leadingZeroBits
    while ((readOneBit() == 0) && (i < 32) && (!isEnd()))
    {
        i++;
    }
    // 2.计算read_bits( leadingZeroBits )
    r = readBits(i);
    // 3.计算codeNum，1 << i即为2的i次幂
    r += (1 << i) - 1;

    return r;
}

int H264Nalu::readSe()
{
    // 1.解码出codeNum，记为r
    int r = readUe();
    // 2.判断r的奇偶性
    if (r & 0x01) // 如果为奇数，说明编码前>0
    {
        r = (r + 1) / 2;
    }
    else // 如果为偶数，说明编码前<=0
    {
        r = -(r / 2);
    }

    return r;
}

int H264Nalu::readTe(int x)
{
    int r = 0;

    // 1.判断取值上限
    if (x == 1) // 如果为1则将读取到的比特值取反
    {
        r = 1 - readOneBit();
    }
    else if (x > 1) // 否则按照ue(v)进行解码
    {
        r = readUe();
    }

    return r;
}

/**
 scaling_list函数实现
 [h264协议文档位置]：7.3.2.1.1 Scaling list syntax
 */
void H264Nalu::scaling_list(int *scalingList, int sizeOfScalingList, int *useDefaultScalingMatrixFlag)
{
    int deltaScale;
    int lastScale = 8;
    int nextScale = 8;

    for (int j = 0; j < sizeOfScalingList; j++)
    {

        if (nextScale != 0)
        {
            deltaScale = readSe();
            nextScale = (lastScale + deltaScale + 256) % 256;
            *useDefaultScalingMatrixFlag = (j == 0 && nextScale == 0);
        }

        scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
        lastScale = scalingList[j];
    }
}

void H264Nalu::getSps()
{
    sps_t *sps =  m_file->activeSps();

    sps->profile_idc = readBits(8);
    sps->constraint_set0_flag = readBits(1);
    sps->constraint_set1_flag = readBits(1);
    sps->constraint_set2_flag = readBits(1);
    sps->constraint_set3_flag = readBits(1);
    sps->constraint_set4_flag = readBits(1);
    sps->constraint_set5_flag = readBits(1);
    sps->reserved_zero_2bits = readBits(2);
    sps->level_idc = readBits(8);

    sps->seq_parameter_set_id = readUe();

    if (sps->profile_idc == 100 || sps->profile_idc == 110 || sps->profile_idc == 122 || sps->profile_idc == 244 || sps->profile_idc == 44 || sps->profile_idc == 83 || sps->profile_idc == 86 || sps->profile_idc == 118 || sps->profile_idc == 128 || sps->profile_idc == 138 || sps->profile_idc == 139 || sps->profile_idc == 134 || sps->profile_idc == 135)
    {

        sps->chroma_format_idc = readUe();
        if (sps->chroma_format_idc == YUV_4_4_4)
        {
            sps->separate_colour_plane_flag = readBits(1);
        }
        sps->bit_depth_luma_minus8 = readUe();
        sps->bit_depth_chroma_minus8 = readUe();
        sps->qpprime_y_zero_transform_bypass_flag = readBits(1);
        sps->seq_scaling_matrix_present_flag = readBits(1);

        if (sps->seq_scaling_matrix_present_flag)
        {
            int scalingListCycle = (sps->chroma_format_idc != YUV_4_4_4) ? 8 : 12;
            for (int i = 0; i < scalingListCycle; i++)
            {
                sps->seq_scaling_list_present_flag[i] = readBits(1);
                if (sps->seq_scaling_list_present_flag[i])
                {
                    if (i < 6)
                    {
                        scaling_list(sps->ScalingList4x4[i], 16, &sps->UseDefaultScalingMatrix4x4Flag[i]);
                    }
                    else
                    {
                        scaling_list(sps->ScalingList8x8[i - 6], 64, &sps->UseDefaultScalingMatrix8x8Flag[i - 6]);
                    }
                }
            }
        }
    }

    sps->log2_max_frame_num_minus4 = readUe();
    sps->pic_order_cnt_type = readUe();
    if (sps->pic_order_cnt_type == 0)
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = readUe();
    }
    else if (sps->pic_order_cnt_type == 1)
    {
        sps->delta_pic_order_always_zero_flag = readBits( 1);
        sps->offset_for_non_ref_pic = readSe();
        sps->offset_for_top_to_bottom_field = readSe();
        sps->num_ref_frames_in_pic_order_cnt_cycle = readUe();
        for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            sps->offset_for_ref_frame[i] = readSe();
        }
    }

    sps->max_num_ref_frames = readUe();
    sps->gaps_in_frame_num_value_allowed_flag = readBits(1);

    sps->pic_width_in_mbs_minus1 = readUe();
    sps->pic_height_in_map_units_minus1 = readUe();
    sps->frame_mbs_only_flag = readBits(1);
    if (!sps->frame_mbs_only_flag)
    {
        sps->mb_adaptive_frame_field_flag = readBits(1);
    }

    sps->direct_8x8_inference_flag = readBits(1);

    sps->frame_cropping_flag = readBits(1);
    if (sps->frame_cropping_flag)
    {
        sps->frame_crop_left_offset = readUe();
        sps->frame_crop_right_offset = readUe();
        sps->frame_crop_top_offset = readUe();
        sps->frame_crop_bottom_offset = readUe();
    }

    sps->vui_parameters_present_flag = readBits(1);
    if (sps->vui_parameters_present_flag)
    {
        parse_vui_parameters(sps);
    }
}

void H264Nalu::getPps() 
{
    pps_t *pps =  m_file->activePps();
     // 解析slice_group_id[]需用的比特个数
    int bitsNumberOfEachSliceGroupID;
    
    pps->pic_parameter_set_id = readUe();
    pps->seq_parameter_set_id = readUe();
    pps->entropy_coding_mode_flag = readBits( 1);
    pps->bottom_field_pic_order_in_frame_present_flag = readBits( 1);
    
    /*  —————————— FMO相关 Start  —————————— */
    pps->num_slice_groups_minus1 = readUe();
    if (pps->num_slice_groups_minus1 > 0) {
        pps->slice_group_map_type = readUe();
        if (pps->slice_group_map_type == 0)
        {
            for (int i = 0; i <= pps->num_slice_groups_minus1; i++) {
                pps->run_length_minus1[i] = readUe();
            }
        }
        else if (pps->slice_group_map_type == 2)
        {
            for (int i = 0; i < pps->num_slice_groups_minus1; i++) {
                pps->top_left[i] = readUe();
                pps->bottom_right[i] = readUe();
            }
        }
        else if (pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5)
        {
            pps->slice_group_change_direction_flag = readBits( 1);
            pps->slice_group_change_rate_minus1 = readUe();
        }
        else if (pps->slice_group_map_type == 6)
        {
            // 1.计算解析slice_group_id[]需用的比特个数，Ceil( Log2( num_slice_groups_minus1 + 1 ) )
            if (pps->num_slice_groups_minus1+1 >4)
                bitsNumberOfEachSliceGroupID = 3;
            else if (pps->num_slice_groups_minus1+1 > 2)
                bitsNumberOfEachSliceGroupID = 2;
            else
                bitsNumberOfEachSliceGroupID = 1;
            
            // 2.动态初始化指针pps.slice_group_id
            pps->pic_size_in_map_units_minus1 = readUe();
            pps->slice_group_id = (int*)calloc(pps->pic_size_in_map_units_minus1+1, 1);
            if (pps->slice_group_id == NULL) {
                fprintf(stderr, "%s\n", "parse_pps_syntax_element slice_group_id Error");
                exit(-1);
            }
            
            for (int i = 0; i <= pps->pic_size_in_map_units_minus1; i++) {
                pps->slice_group_id[i] = readBits( bitsNumberOfEachSliceGroupID);
            }
        }
    }
    /*  —————————— FMO相关 End  —————————— */
    
    pps->num_ref_idx_l0_default_active_minus1 = readUe();
    pps->num_ref_idx_l1_default_active_minus1 = readUe();
    
    pps->weighted_pred_flag = readBits( 1);
    pps->weighted_bipred_idc = readBits( 2);
    
    pps->pic_init_qp_minus26 = readSe( );
    pps->pic_init_qs_minus26 = readSe();
    pps->chroma_qp_index_offset = readSe();
    
    pps->deblocking_filter_control_present_flag = readBits( 1);
    pps->constrained_intra_pred_flag = readBits( 1);
    pps->redundant_pic_cnt_present_flag = readBits( 1);
    
    // 如果有更多rbsp数据
    if (more_rbsp_data()) {
        pps->transform_8x8_mode_flag = readBits( 1);
        pps->pic_scaling_matrix_present_flag = readBits( 1);
        if (pps->pic_scaling_matrix_present_flag) {
            int chroma_format_idc = m_file->Sequence_Parameters_Set_Array[pps->seq_parameter_set_id].chroma_format_idc;
            int scalingListCycle = 6 + ((chroma_format_idc != YUV_4_4_4) ? 2 : 6) * pps->transform_8x8_mode_flag;
            for (int i = 0; i < scalingListCycle; i++) {
                pps->pic_scaling_list_present_flag[i] = readBits( 1);
                if (pps->pic_scaling_list_present_flag[i]) {
                    if (i < 6) {
                        scaling_list(pps->ScalingList4x4[i], 16, &pps->UseDefaultScalingMatrix4x4Flag[i]);
                    }else {
                        scaling_list(pps->ScalingList8x8[i-6], 64, &pps->UseDefaultScalingMatrix8x8Flag[i]);
                    }
                }
            }
        }
        pps->second_chroma_qp_index_offset = readSe();
    }else {
        pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
    }
}

/**
 解析vui_parameters()句法元素
 [h264协议文档位置]：Annex E.1.1
 */
void H264Nalu::parse_vui_parameters(sps_t *sps)
{
    sps->vui_parameters.aspect_ratio_info_present_flag = readBits(1);
    if (sps->vui_parameters.aspect_ratio_info_present_flag)
    {
        sps->vui_parameters.aspect_ratio_idc = readBits(8);
        if (sps->vui_parameters.aspect_ratio_idc == 255)
        { // Extended_SAR值为255
            sps->vui_parameters.sar_width = readBits(16);
            sps->vui_parameters.sar_height = readBits(16);
        }
    }

    sps->vui_parameters.overscan_info_present_flag = readBits(1);
    if (sps->vui_parameters.overscan_info_present_flag)
    {
        sps->vui_parameters.overscan_appropriate_flag = readBits(1);
    }

    sps->vui_parameters.video_signal_type_present_flag = readBits(1);
    if (sps->vui_parameters.video_signal_type_present_flag)
    {
        sps->vui_parameters.video_format = readBits(3);
        sps->vui_parameters.video_full_range_flag = readBits(1);

        sps->vui_parameters.colour_description_present_flag = readBits(1);
        if (sps->vui_parameters.colour_description_present_flag)
        {
            sps->vui_parameters.colour_primaries = readBits(8);
            sps->vui_parameters.transfer_characteristics = readBits(8);
            sps->vui_parameters.matrix_coefficients = readBits(8);
        }
    }

    sps->vui_parameters.chroma_loc_info_present_flag = readBits(1);
    if (sps->vui_parameters.chroma_loc_info_present_flag)
    {
        sps->vui_parameters.chroma_sample_loc_type_top_field = readUe();
        sps->vui_parameters.chroma_sample_loc_type_bottom_field = readUe();
    }

    sps->vui_parameters.timing_info_present_flag = readBits(1);
    if (sps->vui_parameters.timing_info_present_flag)
    {
        sps->vui_parameters.num_units_in_tick = readBits(32);
        sps->vui_parameters.time_scale = readBits(32);
        sps->vui_parameters.fixed_frame_rate_flag = readBits(1);
    }

    sps->vui_parameters.nal_hrd_parameters_present_flag = readBits(1);
    if (sps->vui_parameters.nal_hrd_parameters_present_flag)
    {
        parse_vui_hrd_parameters(&sps->vui_parameters.nal_hrd_parameters);
    }

    sps->vui_parameters.vcl_hrd_parameters_present_flag = readBits(1);
    if (sps->vui_parameters.vcl_hrd_parameters_present_flag)
    {
        parse_vui_hrd_parameters(&sps->vui_parameters.vcl_hrd_parameters);
    }

    if (sps->vui_parameters.nal_hrd_parameters_present_flag ||
        sps->vui_parameters.vcl_hrd_parameters_present_flag)
    {
        sps->vui_parameters.low_delay_hrd_flag = readBits(1);
    }

    sps->vui_parameters.pic_struct_present_flag = readBits(1);
    sps->vui_parameters.bitstream_restriction_flag = readBits(1);
    if (sps->vui_parameters.bitstream_restriction_flag)
    {
        sps->vui_parameters.motion_vectors_over_pic_boundaries_flag = readBits(1);
        sps->vui_parameters.max_bytes_per_pic_denom = readUe();
        sps->vui_parameters.max_bits_per_mb_denom = readUe();
        sps->vui_parameters.log2_max_mv_length_horizontal = readUe();
        sps->vui_parameters.log2_max_mv_length_vertical = readUe();
        sps->vui_parameters.max_num_reorder_frames = readUe();
        sps->vui_parameters.max_dec_frame_buffering = readUe();
    }
}

/**
 解析hrd_parameters()句法元素
 [h264协议文档位置]：Annex E.1.2
 */
void H264Nalu::parse_vui_hrd_parameters(hrd_parameters_t *hrd)
{
    hrd->cpb_cnt_minus1 = readUe();
    hrd->bit_rate_scale = readBits(4);
    hrd->cpb_size_scale = readBits(4);

    for (int SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++)
    {
        hrd->bit_rate_value_minus1[SchedSelIdx] = readUe();
        hrd->cpb_size_value_minus1[SchedSelIdx] = readUe();
        hrd->cbr_flag[SchedSelIdx] = readBits(1);
    }

    hrd->initial_cpb_removal_delay_length_minus1 = readBits(5);
    hrd->cpb_removal_delay_length_minus1 = readBits(5);
    hrd->dpb_output_delay_length_minus1 = readBits(5);
    hrd->time_offset_length = readBits(5);
}

/**
 在rbsp_trailing_bits()之前是否有更多数据
 [h264协议文档位置]：7.2 Specification of syntax functions, categories, and descriptors
 */
int H264Nalu::more_rbsp_data()
{
    // 0.是否已读到末尾
    if (isEnd()) {return 0;}
    
    // 1.下一比特值是否为0，为0说明还有更多数据
    if (peekOneBit() == 0) {return 1;}
    
    // 2.到这说明下一比特值为1，这就要看看是否这个1就是rbsp_stop_one_bit，也即1后面是否全为0
    H264Nalu bs_temp(*this);
    
    // 3.跳过刚才读取的这个1，逐比特查看1后面的所有比特，直到遇到另一个1或读到结束为止
    bs_temp.readOneBit();
    while(!bs_temp.isEnd())
    {
        if (bs_temp.readOneBit() == 1) { return 1; }
    }
    
    return 0;
}