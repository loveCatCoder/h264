#ifndef _H264_BUFFER_H_
#define _H264_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include "H264Private.h"
#include "H264File.h"

class H264Buffer
{
private:
    /* data */
    uint8_t *start;
    uint8_t *p;
    uint8_t *end;
    int bits_left;
    H264File *m_file;

public:
    H264Buffer(H264File *file);
    H264Buffer(const H264Buffer &buf);
    ~H264Buffer();

public:
    void getSps();
    void getPps();

    bool isEnd();
    uint8_t peekOneBit();
    uint8_t readOneBit();
    int readBits(int n);
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
