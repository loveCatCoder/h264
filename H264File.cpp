
#include "H264File.h"
#include "H264Nalu.h"
#include <assert.h>

H264File::~H264File()
{
    free(m_fileBuffer);
}

int H264File::findNalu()
{
    //( next_bits( 24 ) != 0x000001 && next_bits( 32 ) != 0x00000001 )
    // 寻找起始码，只要有一位不满足，则继续向下寻找
    while (
        (m_fileBuffer[m_naluIndex] != 0x00 || m_fileBuffer[m_naluIndex + 1] != 0x00 || m_fileBuffer[m_naluIndex + 2] != 0x01) &&
        (m_fileBuffer[m_naluIndex] != 0x00 || m_fileBuffer[m_naluIndex + 1] != 0x00 || m_fileBuffer[m_naluIndex + 2] != 0x00 || m_fileBuffer[m_naluIndex + 3] != 0x01))
    {

        m_naluIndex = m_naluIndex + 1;
        if (m_naluIndex + 3 > m_fileSize)
        {
            m_nalu.len = 0;
            return 0;
        } // 没有找到，退出函数
    }

    // 找到起始码，判断如果不是0x000001，则是0x00000001，则将读取索引加1
    // if( next_bits( 24 ) != 0x000001 )
    if (m_fileBuffer[m_naluIndex] != 0x00 || m_fileBuffer[m_naluIndex + 1] != 0x00 || m_fileBuffer[m_naluIndex + 2] != 0x01)
    {

        m_naluIndex = m_naluIndex + 1; // 读取索引加1
    }
    //index 为跳过0x000001或者0x00000001的位置
    m_naluIndex += 3; // 读取索引加3
    m_naluStart = m_naluIndex;

    // 到达nalu部分
    int j = 0;
    // 寻找结尾
    //( next_bits( 24 ) != 0x000000 && next_bits( 24 ) != 0x000001 )
    while (
        (m_fileBuffer[m_naluIndex] != 0x00 || m_fileBuffer[m_naluIndex + 1] != 0x00 || m_fileBuffer[m_naluIndex + 2] != 0x00) &&
        (m_fileBuffer[m_naluIndex] != 0x00 || m_fileBuffer[m_naluIndex + 1] != 0x00 || m_fileBuffer[m_naluIndex + 2] != 0x01))
    {

        m_nalu.buf[j] = m_fileBuffer[m_naluIndex]; // 将读取到的nalu存放在全局变量nalu当中
        j++;
        m_naluIndex = m_naluIndex + 1;
        if (m_naluIndex + 3 >= m_fileSize)
        { // 寻找到文件结尾

            m_nalu.buf[j] = m_fileBuffer[m_naluIndex];
            m_nalu.buf[j + 1] = m_fileBuffer[m_naluIndex + 1];
            m_nalu.buf[j + 2] = m_fileBuffer[m_naluIndex + 2];
            m_nalu.buf[j + 3] = m_fileBuffer[m_naluIndex + 3];
            m_nalu.len = m_fileSize - m_naluStart;
            m_naluNumber++;
            return m_nalu.len;
        }
    }
    m_naluNumber++;
    m_nalu.len = m_naluIndex - m_naluStart;
    return m_nalu.len;
}

void H264File::readNalu()
{
    int nalu_size = m_nalu.len;

    // 1.去除nalu中的emulation_prevention_three_byte：0x03
    m_nalu.len = naluToRbsp();

    // 2.初始化逐比特读取工具句柄
    H264Buffer bs = H264Buffer(this);

    // 3. 读取nal header 7.3.1
    m_nalu.forbidden_zero_bit = bs.readBits(1);
    m_nalu.nal_ref_idc = bs.readBits(2);
    m_nalu.nal_unit_type = bs.readBits(5);

    printf("naluNumber: %d\n", m_naluNumber);
    printf("\tnal->forbidden_zero_bit: %d\n", m_nalu.forbidden_zero_bit);
    printf("\tnal->nal_ref_idc: %d\n", m_nalu.nal_ref_idc);
    printf("\tnal->nal_unit_type: %d\n", m_nalu.nal_unit_type);

#if TRACE
    fprintf(trace_fp, "\n\nAnnex B m_nalu len %d, forbidden_bit %d, nal_reference_idc %d, nal_unit_type %d\n\n", nalu_size, m_nalu.forbidden_zero_bit, m_nalu.nal_ref_idc, m_nalu.nal_unit_type);
    fflush(trace_fp);
#endif

    switch (m_nalu.nal_unit_type)
    {
    case H264_NAL_SPS:
        m_nalu.len = rbspToSodb();
        bs.getSps();
        break;

    case H264_NAL_PPS:
        m_nalu.len = rbspToSodb();
        bs.getPps();
        break;

    case H264_NAL_SLICE:
        // case H264_NAL_IDR_SLICE:
        //     currentSlice->idr_flag = (m_nalu.nal_unit_type == H264_NAL_IDR_SLICE);
        //     currentSlice->nal_ref_idc = m_nalu.nal_ref_idc;
        //     m_nalu.len = rbsp_to_sodb(m_nalu);
        //     processSlice(bs);
        //     break;

    case H264_NAL_DPA:
        m_nalu.len = rbspToSodb();
        break;

    case H264_NAL_DPB:
        m_nalu.len = rbspToSodb();
        break;

    case H264_NAL_DPC:
        m_nalu.len = rbspToSodb();
        break;

    default:
        break;
    }
}

H264Nalu *H264File::getNalu()
{
    return &m_nalu;
}


int H264File::naluNumber()
{
    return m_naluNumber;
}

sps_t *H264File::activeSps()
{
    return &m_activeSps;
}

pps_t *H264File::activePps()
{
    return &m_activePps;
}

bool H264File::decodeMediaInfo(uint8_t *buffer, int len, H264MediaInfo *info)
{
    //设置buffer
    m_fileBuffer = buffer;
    m_fileSize = len;
    //解析nalu
    while (findNalu() > 0)
    {
        // 读取/解析 nalu
        readNalu();
    }

    //计算参数并返回
    m_mediaInfo.videoWidth = (m_activeSps.pic_width_in_mbs_minus1+1)*16;
    m_mediaInfo.videoHeight = (m_activeSps.pic_height_in_map_units_minus1+1)*16;
    if(m_activeSps.vui_parameters.timing_info_present_flag == 1)
    {
        if(m_activeSps.vui_parameters.num_units_in_tick != 0)
        {
            m_mediaInfo.frameRate = m_activeSps.vui_parameters.time_scale/(2*m_activeSps.vui_parameters.num_units_in_tick);
        }
    }
    else
    {
        m_mediaInfo.frameRate = 0;
    }
    
    info->frameRate = m_mediaInfo.frameRate;
    info->videoHeight = m_mediaInfo.videoHeight;
    info->videoWidth = m_mediaInfo.videoWidth;
    return true;
}

H264File::H264File(const std::string name) : m_fileName(name)
{
    FILE *fp_h264 = fopen(m_fileName.c_str(), "rb");
    if (fp_h264 == NULL)
    {
        printf("打开h264文件失败\n");
        exit(-1);
    }

    m_fileBuffer = (uint8_t *)malloc(MAX_BUFFER_SIZE);
    m_fileSize = (int)fread(m_fileBuffer, sizeof(uint8_t), MAX_BUFFER_SIZE, fp_h264);
    fclose(fp_h264);

    // 1. 开辟nalu_t保存nalu_header和SODB
    // m_nalu = H264Nalu();
    // m_nalu.buf = (uint8_t *)calloc(m_fileSize, sizeof(uint8_t));
    // if (m_nalu.buf == NULL)
    // {
    //     fprintf(stderr, "%s\n", "AllocNALU: m_nalu.buf");
    //     exit(-1);
    // }
}
