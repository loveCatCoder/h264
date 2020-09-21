

#include "H264Nalu.h"
#include "H264File.h"
#include "stdio.h"
#include "malloc.h"

int main()
{
    H264File h264File = H264File("pricess.264");

    // 2.找到h264码流中的各个nalu
    while (h264File.findNalu() > 0)
    {
        // 读取/解析 nalu
        h264File.readNalu();
    }

    return 0;
}