#ifndef _H264_NALU_H_
#define _H264_NALU_H_

#include "h264_private.h"
#include <stdint.h>

/* 7.4.1 Table 7-1 NAL unit types */
enum nal_unit_type
{
    H264_NAL_UNKNOWN = 0,
    H264_NAL_SLICE = 1,
    H264_NAL_DPA = 2,
    H264_NAL_DPB = 3,
    H264_NAL_DPC = 4,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_AUD = 9,
    H264_NAL_END_SEQUENCE = 10,
    H264_NAL_END_STREAM = 11,
    H264_NAL_FILLER_DATA = 12,
    H264_NAL_SPS_EXT = 13,
    H264_NAL_AUXILIARY_SLICE = 19,
};

class H264Nalu
{
public:
    // nal unit header
    int forbidden_zero_bit; // f(1)
    int nal_ref_idc;        // u(2)
    int nal_unit_type;      // u(5)
    int len;                // 最开始保存nalu_size, 然后保存rbsp_size,最终保存SODB的长度
    uint8_t *buf;
    // nal unit body
    
public:
    sps_t *m_activeSps;
    pps_t *m_activePps;

public:
    H264Nalu(/* args */);
    ~H264Nalu();
};



#endif