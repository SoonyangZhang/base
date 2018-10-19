#include "H264Decoder.h"
 
H264Decoder::H264Decoder(void)
{
}
 
 
H264Decoder::~H264Decoder(void)
{
}
 
X264_DECODER_H H264Decoder::X264Decoder_Init()
{
    X264_Decoder_Handle *pHandle = (X264_Decoder_Handle *)malloc(sizeof(X264_Decoder_Handle));
    if (pHandle == NULL)
    {
        return -1;
    }
    
    avcodec_register_all();
    
    
    
    pHandle->codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!pHandle->codec)
    {
        return -2;
    }
    
    pHandle->c = avcodec_alloc_context3(pHandle->codec);
    if (!pHandle->c)
    {
        return -3;
    }
    
    pHandle->c->codec_type = AVMEDIA_TYPE_VIDEO;
    pHandle->c->pix_fmt =AV_PIX_FMT_YUV420P;
	pHandle->c->flags2|=AV_CODEC_FLAG2_CHUNKS;
 
	pHandle->pCodecParserCtx=av_parser_init(AV_CODEC_ID_H264);  
    if (!pHandle->pCodecParserCtx){  
        printf("Could not allocate video parser context\n");  
        return -6;  
    }  
 
    if (avcodec_open2(pHandle->c, pHandle->codec, NULL) < 0)
    {
        return -4;
    }
    
    pHandle->picture =av_frame_alloc();
    if (!pHandle->picture)
    {
        return -5;
    }
    
    pHandle->frame_count = 0;
    
    return (X264_DECODER_H)pHandle;
}
void H264Decoder::X264Decoder_UnInit(X264_DECODER_H dwHandle)
{
    if (dwHandle <= 0)
    {
        return;
    }
    
    X264_Decoder_Handle *pHandle = (X264_Decoder_Handle *)dwHandle;
    
    avcodec_close(pHandle->c);
    av_free(pHandle->c);
	//av_free(pHandle->picture->data[0]);//segment fault
    av_free(pHandle->picture);
    
    free(pHandle);
}
int H264Decoder::X264Decoder_Decode(X264_DECODER_H dwHandle, uint8_t *pDataIn, int nInSize, uint8_t *pDataOut, int *nOutSize, int *nWidth, int *nHeight)
{
    if (dwHandle <= 0)
    {
        return -1;
    }
	//unsigned char buf[20*1024]={0};
	
	
    X264_Decoder_Handle *pHandle = (X264_Decoder_Handle *)dwHandle;
 
	av_init_packet(&(pHandle->avpkt));
 
	pHandle->avpkt.size = nInSize;
	pHandle->avpkt.data = pDataIn;
 
    while (pHandle->avpkt.size > 0)
    {
        pHandle->comsumedSize = avcodec_decode_video2(pHandle->c, pHandle->picture, &pHandle->got_picture, &(pHandle->avpkt));
        if (pHandle->comsumedSize < 0)
        {
            return -2;
        }
        
        if (pHandle->got_picture)
        {
			
            *nWidth = pHandle->c->width;
			
            *nHeight = pHandle->c->height;
			
			if(pHandle->c->width!=0 && pHandle->c->height!=0)
				{*nOutSize=pHandle->c->width*pHandle->c->height*3/2;}
			else
			    {*nOutSize=0;}
			
            pgm_save2(pHandle->picture->data[0],
                      pHandle->picture->linesize[0],
                      pHandle->c->width,
                      pHandle->c->height,pDataOut);
            pgm_save2(pHandle->picture->data[1],
                      pHandle->picture->linesize[1],
                      pHandle->c->width/2,
                      pHandle->c->height/2,
                      pDataOut +pHandle->c->width * pHandle->c->height);
            pgm_save2(pHandle->picture->data[2],
                      pHandle->picture->linesize[2],
                      pHandle->c->width/2,
                      pHandle->c->height/2,
                      pDataOut +pHandle->c->width * pHandle->c->height*5/4);
        }
        
        if (pHandle->avpkt.data)
        {
            pHandle->avpkt.size -= pHandle->comsumedSize;
            pHandle->avpkt.data += pHandle->comsumedSize;
        }
    }
   
    return 0;
}
