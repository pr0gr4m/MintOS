/*
 * JPEG decoding engine for DCT-baseline
 *
 *      JPEGLS - Copyright(c) 2004 by Hajime Uchimura(nikutama@gmail.com)
 * 
 * history::
 * 2003/04/28 | added OSASK-GUI ( by H.Kawai ) 
 * 2003/05/12 | optimized DCT ( 20-bits fixed point, etc...) -> line 407-464 ( by I.Tak. )
 * 2009/11/21 | optimized to RGB565 ( by kkamagui )
 * 2018/03/13 | optimized character set ( by pr0gr4m )
 */

#ifndef __JPEG_H__
#define __JPEG_H__

#include "Types.h"
#include "2DGraphics.h"

typedef struct{
    int elem;
    unsigned short code[256];
    unsigned char  size[256];
    unsigned char  value[256];
}HUFF;

typedef struct{
    // SOF
    int width;
    int height;
    // MCU
    int mcu_width;
    int mcu_height;

    int max_h,max_v;
    int compo_count;
    int compo_id[3];
    int compo_sample[3];
    int compo_h[3];
    int compo_v[3];
    int compo_qt[3];

    // SOS
    int scan_count;
    int scan_id[3];
    int scan_ac[3];
    int scan_dc[3];
    int scan_h[3];
    int scan_v[3];
    int scan_qt[3];
    
    // DRI
    int interval;

    int mcu_buf[32*32*4];
    int *mcu_yuv[4];
    int mcu_preDC[3];
    
    // DQT
    int dqt[3][64];
    int n_dqt;
    
    // DHT
    HUFF huff[2][3];
    
    
    // i/o
    unsigned char *data;
    int data_index;
    int data_size;
    
    unsigned long bit_buff;
    int bit_remain;
    
}JPEG;


BOOL kJPEGInit(JPEG *jpeg, BYTE* pbFileBuffer, DWORD dwFileSize);
BOOL kJPEGDecode(JPEG *jpeg, COLOR* rgb);

#endif /*__JPEG_H__*/
