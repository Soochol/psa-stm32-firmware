// 골든 픽스처 생성기 — 펌웨어와 동일한 lib_crc.c를 링크한다.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lib_crc.h"

#define REC 80
#define HDR 512
static void be16(uint8_t* p, uint16_t v){ p[0]=v>>8; p[1]=v; }
static void be32(uint8_t* p, uint32_t v){ p[0]=v>>24; p[1]=v>>16; p[2]=v>>8; p[3]=v; }

static void hdr_build(uint8_t* h, uint32_t bootId, uint32_t fileIdx, uint32_t firstSeq){
    memset(h,0,HDR); memcpy(h+0,"PSA1",4);
    be16(h+4,1); be16(h+6,REC);
    memset(h+8,0xFF,6);                       // deviceId 미수신
    be32(h+14,bootId); be32(h+18,fileIdx); be32(h+22,firstSeq); be32(h+26,10000);
    be16(h+30, u16_CRC16_CCITT(h,30));
}
static void rec_build(uint8_t* r, uint32_t seq, uint32_t tick, const uint8_t* pl,
                      uint8_t mode, uint8_t flags, uint16_t err){
    memset(r,0,REC);
    be32(r+0,seq); be32(r+4,tick);
    if(pl) memcpy(r+8,pl,64);
    r[72]=mode; r[73]=flags; be16(r+74,err);
    be16(r+78, u16_CRC16_CCITT(r,78));
}
static void dump(const char* name, const uint8_t* b, int n){
    printf("%s (%d B)\n", name, n);
    for(int i=0;i<n;i++){ if(i%16==0) printf("  %04X: ",i); printf("%02X ",b[i]);
        if(i%16==15||i==n-1) printf("\n"); }
}
int main(void){
    uint8_t h[HDR], r[REC], pl[64];
    FILE* f = fopen("golden.psa","wb");

    hdr_build(h, 42, 0, 0);
    dump("[헤더 앞 32 B] bootId=42 fileIndex=0 firstSeq=0 deviceId=FF..", h, 32);
    fwrite(h,1,HDR,f);

    for(int i=0;i<64;i++) pl[i]=(uint8_t)i;    // 0x00..0x3F 패턴
    rec_build(r, 0, 1000, pl, 7 /*HEALING*/, 0x01 /*TX_OK*/, 0x0000);
    dump("\n[레코드 0] seq=0 tick=1000 mode=HEALING(7) flags=TX_OK", r, REC);
    fwrite(r,1,REC,f);

    rec_build(r, 1, 1100, pl, 1 /*WAITING*/, 0x01, 0x0200 /*ERR_SD_CARD*/);
    dump("\n[레코드 1] seq=1 tick=1100 mode=WAITING(1) errorMask=0x0200", r, REC);
    fwrite(r,1,REC,f);

    rec_build(r, 2, 1200, NULL, 1, 0x02 /*INVALID*/, 0x0000);
    dump("\n[레코드 2 = 플레이스홀더] seq=2 flags=INVALID(bit1) payload=0x00", r, REC);
    fwrite(r,1,REC,f);

    fclose(f);
    printf("\n파일 크기 = %d + %d x 3 = %d B\n", HDR, REC, HDR+REC*3);
    return 0;
}
