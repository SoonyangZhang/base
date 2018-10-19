/*
* https://blog.csdn.net/cjj198561/article/details/30248963
*/
extern "C"{
#include "libavformat/avformat.h"
 
#include "libavutil/pixfmt.h"
#include "libavcodec/avcodec.h"
 
 
}
#define X264_DECODER_H long
 
typedef struct
{
    struct AVCodec *codec;// Codec
    struct AVCodecContext *c;// Codec Context
	AVCodecParserContext *pCodecParserCtx;
    int frame_count;
    struct AVFrame *picture;// Frame
    AVPacket avpkt;
    
    int iWidth;
    int iHeight;
    int comsumedSize;
    int got_picture;
} X264_Decoder_Handle;
 
class H264Decoder
{
public:
	H264Decoder(void);
	~H264Decoder(void);
	public:
    // H264解码初期化
    X264_DECODER_H X264Decoder_Init();
    
    /*
     H264数据解码，解码后的数据为yuv420格式
     dwHandle:初期化时返回的句柄
     pDataIn:待解码数据
     nInSize:待解码数据长度
     pDataOut:解码后的数据，存储空间由调用者申请
     nOutSize:存储空间大小
     
     nWidth:解码后的图像宽度
     nHeight:解码后的图像高度
     */
    int X264Decoder_Decode(X264_DECODER_H dwHandle, uint8_t *pDataIn, int nInSize, uint8_t *pDataOut, int *nOutSize, int *nWidth,int *nHeight);
	/*
     H264解码销毁
     dwHandle:初期化时返回的句柄
     */
    void X264Decoder_UnInit(X264_DECODER_H dwHandle);
    
protected:
    void pgm_save2(unsigned char *buf, int wrap, int xsize, int ysize, uint8_t *pDataOut)
    {
        int i;
        for(i = 0; i < ysize; i++)
        {
            memcpy(pDataOut+i*xsize, buf + /*(ysize-i)*/i * wrap, xsize);
        }
    }
};
