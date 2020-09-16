#ifndef _H264_FILE_H_
#define _H264_FILE_H_

#include <stdint.h>
#include <stdio.h>
#include <string>
#include "h264_nalu.h"

class H264File
{
private:
    static const uint MAX_BUFFER_SIZE = 5 * 1024 * 1024;

private:
    uint8_t *m_fileBuffer;
    std::string m_fileName;
    int m_fileSize;
    H264Nalu m_nalu;

    int m_naluNumber = 0;
    int m_naluStart = 0; // 当前找到的nalu起始位置
    int m_naluIndex = 0; // 当前查找的位置索引

public:
    H264File(const std::string name);
    ~H264File();

public:
    int findNalu();
    void readNalu();
    
    int naluToRbsp();
    int rbspToSodb();

    int naluNumber();

};

#endif