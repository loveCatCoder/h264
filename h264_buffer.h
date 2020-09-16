#ifndef _H264_BUFFER_H_
#define _H264_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include "h264_private.h"


class H264Buffer
{
private:
    /* data */
    uint8_t *start;
    uint8_t *p;
    uint8_t *end;
    int bits_left;
    // 因为sps_id的取值范围为[0,31]，因此数组容量最大为32，详见7.4.2.1
    sps_t Sequence_Parameters_Set_Array[32];
    // 因为pps_id的取值范围为[0,255]，因此数组容量最大为256，详见7.4.2.2
    pps_t Picture_Parameters_Set_Array[256];

public:
    H264Buffer(uint8_t *buffer, size_t size);
    H264Buffer(const H264Buffer &buf);
    ~H264Buffer();

public:
    sps_t *getSps();
    pps_t *getPps();

    bool isEnd();
    uint8_t peekOneBit();
    uint8_t readOneBit();
    int readBit(int n);
    int readUe();
    int readSe();
    int readTe(int x);
private:
    void scaling_list(int *scalingList, int sizeOfScalingList, int *useDefaultScalingMatrixFlag);
    void parse_vui_parameters(sps_t *sps);
    void parse_vui_hrd_parameters(hrd_parameters_t *hrd);
    int more_rbsp_data();

    
};

#endif
