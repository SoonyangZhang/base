#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "echo_h264_encoder.h"
#define IMAGE_WIDTH  640
#define IMAGE_HEIGHT 480
#define YUV_FRAME_SIZE 640*480*3/2
#define MAX_PIC_SIZE 1024000
#define FAIL_IF_ERROR(cond, ...) \
do\
{\
    if(cond)\
    {\
        fprintf(stderr, __VA_ARGS__);\
        abort();\
    }\
}while(0)
int main(int argc, char **argv){
	int height=480,width=640;
   // FAIL_IF_ERROR(!(argc > 1), "x264_encoder usage:x264_encoder widthxheight <input.yuv> output.264\n");
    FAIL_IF_ERROR(2 != sscanf(argv[1], "%dx%d", &width, &height), "resolution not sepcified or incorrect\n");	
	H264Encoder encoder;
	uint8_t buffer[YUV_FRAME_SIZE];
	FILE *fp_src;
    FILE *fp_dst;
    fp_src = fopen(argv[2], "rb");
    if(!fp_src){
        fprintf(stderr, "open input yuv file faile\n");
    }

    fp_dst = fopen(argv[3], "wb");
    if(!fp_src){
        fprintf(stderr, "open output h264 file faile\n");
    }
	bool flag=false;
	flag=encoder.init(30,width,height,width,height);
	if(!flag){perror("init encoder error"); abort();}
	//encoder.set_bitrate(300);
	int frame_type=0;
	int out_size=0;
	uint8_t outbuffer[MAX_PIC_SIZE];
	/*outbuffer=(uint8_t*)malloc(MAX_PIC_SIZE*sizeof(uint8_t));
	if(!outbuffer){
		perror("buffer allocate failed");
		return -1; 
	}*/
	int counter =0;
	while(!feof(fp_src)){
		if(fread(buffer,1,YUV_FRAME_SIZE,fp_src)!=YUV_FRAME_SIZE){
			break;
		}
		flag=encoder.encode(buffer,YUV_FRAME_SIZE,AV_PIX_FMT_YUV420P,
		outbuffer,&out_size,&frame_type,false);
		if(!flag){
		perror("encoder error");
		break;
		}
		if(!fwrite(outbuffer,out_size,1,fp_dst)){
			perror("write encoded data failed");
			break;
		}
		if(counter==100)
		{
		printf("change bitrate\n");
		encoder.set_bitrate(200);
		}
		counter++;
	}
	printf("%d\n",counter);
	//free(outbuffer);
	return 0;
}
