#ifndef _H264_FILE_H_
#define _H264_FILE_H_

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "H264Nalu.h"


class H264File
{
private:
    static const uint MAX_BUFFER_SIZE = 5 * 1024 * 1024;

private:
    uint8_t *m_fileBuffer;
    std::string m_fileName;
    int m_fileSize;

    int m_naluNumber = 0;
    int m_naluStart = 0; // 当前找到的nalu起始位置
    int m_naluIndex = 0; // 当前查找的位置索引

    sps_t m_activeSps;
    pps_t m_activePps;
    H264MediaInfo m_mediaInfo;
public:
    // 因为sps_id的取值范围为[0,31]，因此数组容量最大为32，详见7.4.2.1
    sps_t Sequence_Parameters_Set_Array[32];
    // 因为pps_id的取值范围为[0,255]，因此数组容量最大为256，详见7.4.2.2
    pps_t Picture_Parameters_Set_Array[256];

public:
    H264File(const std::string name);
    ~H264File();

public:
    int findNalu();
    void readNalu();

    int naluNumber();
    sps_t* activeSps();
    pps_t* activePps();
public:
    bool decodeMediaInfo(uint8_t *buffer,int len,H264MediaInfo *info);
};

#endif