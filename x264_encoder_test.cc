#include <stdint.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string>
#include <utility>
#include <memory>
#include "cmdline.h"
#include "echo_h264_encoder.h"
#define MAX_PIC_SIZE 1024000
/*
ffmpeg -s 1280x720 -i mobcal.yuv -vcodec libx264  -b:v 2M -maxrate 2M -bufsize 1M -x264-params bframes=0 rate-2M.h264 
./encoder -w 1280 -h 720 -i ../data/mobcal.yuv -o 1.5M.h264 -b 1500
*/
using namespace std;
int main(int argc, char **argv){
    cmdline::parser a;
    a.add<int>("width", 'w', "frame width", true,1280, cmdline::range(1, 65535));
    a.add<int>("height", 'h', "frame height", true,720, cmdline::range(1, 65535));
    a.add<int>("bitrate", 'b', "encode rate", true,2500, cmdline::range(500,4000));//500kbps~4Mbps
    
    a.add<string>("input", 'i', "input", true, "xx.yuv");
    a.add<string>("output", 'o', "output", false, "yy.h264");
    a.parse_check(argc, argv);
	int width=a.get<int>("width");
    int height=a.get<int>("height");
    int bitrate=a.get<int>("bitrate");
    std::string fileIn=a.get<string>("input");
    std::string fileOut=a.get<string>("output");
    std::cout<<width<<" "<<height<<std::endl;	
	H264Encoder encoder;
    int frameSize=width*height*3/2;
	std::unique_ptr<uint8_t[]> buffer(new uint8_t[frameSize]);
	FILE *fp_src;
    FILE *fp_dst;
    fp_src = fopen(fileIn.c_str(), "rb");
    if(!fp_src){
        fprintf(stderr, "open input yuv file faile\n");
    }

    fp_dst = fopen(fileOut.c_str(), "wb");
    if(!fp_src){
        fprintf(stderr, "open output h264 file faile\n");
    }
	bool flag=false;
	flag=encoder.init(30,width,height,width,height,bitrate);
	if(!flag){perror("init encoder error"); abort();}
	int frame_type=0;
	int out_size=0;
	uint8_t outbuffer[MAX_PIC_SIZE];
	int counter =0;
	while(!feof(fp_src)){
		if(fread(buffer.get(),1,frameSize,fp_src)!=frameSize){
			break;
		}
		flag=encoder.encode(buffer.get(),frameSize,AV_PIX_FMT_YUV420P,
		outbuffer,&out_size,&frame_type,false);
		if(!flag){
		perror("encoder error");
		break;
		}
		if(!fwrite(outbuffer,out_size,1,fp_dst)){
			perror("write encoded data failed");
			break;
		}
		counter++;
	}
	std::cout<<"frames "<<counter<<std::endl;
	return 0;
}
