#include <stdio.h>
#include <string>
#include <utility>
#include <memory>
#include "camera.h"
#include "bw_reader.h"
#include "cmdline.h"
#include "echo_h264_encoder.h"
#include "h264_decoder.h"
#include "psnr.h"
#define MAX_PIC_SIZE 1024000
//./t_test -w 1280 -h 720 -i ../yuv/mobcal.yuv -t ../traces/mab_1_bw.txt -c 6
using namespace std;
int main(int argc, char **argv){
    cmdline::parser a;
    a.add<int>("width", 'w', "frame width", true,1280, cmdline::range(1, 65535));
    a.add<int>("height", 'h', "frame height", true,720, cmdline::range(1, 65535));
    a.add<string>("input", 'i', "input", true, "xx.yuv");
    a.add<string>("bwtrace", 't', "bandwidth trace", true, "xx.bw");
    a.add<int>("column", 'c', "bandwidth sample in which column", true,5, cmdline::range(1, 10));
    a.add<string>("output", 'o', "output", false, "yy.h264");
    a.add<string>("dst", 'd', "yuv output", false, "dst.yuv");
    a.add<string>("quality", 'q', "video quality", false, "xx_psnr.txt");
    a.parse_check(argc, argv);
    int width=a.get<int>("width");
    int height=a.get<int>("height");
    int column=a.get<int>("column");
    std::string fileIn=a.get<string>("input");
    std::string bwTrace=a.get<string>("bwtrace");
    std::string encodeOut=a.get<string>("output");
    std::string yuvOut=a.get<string>("dst");
    std::string qualityOut=a.get<string>("quality");
    FILE *fp_encode=nullptr,*fp_yuv=nullptr,*fp_q=nullptr;
    fp_encode= fopen(encodeOut.c_str(), "wb");
    //fp_yuv=fopen(yuvOut.c_str(), "wb");
    fp_q=fopen(qualityOut.c_str(), "w+");
    int fps=30;
    Camera camera(width,height,fps,fileIn);
    BandwidthReader reader(bwTrace,column);
    H264Encoder encoder;
    H264Decoder decoder;
    X264_DECODER_H handler=decoder.X264Decoder_Init();;
    int bufSize=width*height*3/2;
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[bufSize]);
    uint8_t encodebuffer[MAX_PIC_SIZE];
    std::unique_ptr<uint8_t[]> decodebuffer(new uint8_t[bufSize*2]);
    int totalFrames=camera.getFrames();
    int intendRead=totalFrames+3;
    int gap=1000/fps;
    int totalTick=0;
    int tick=0;
    int readSize=0;
    int lines=reader.getLines();
    totalTick=100*500;
    double bw=reader.getBandwidth();
    reader.Update(0);
	bool flag=false;
	int frame_type=0;
	int out_size=0;
	int kbps=bw*1000;
	int frameCount=0;
	int writeDown=1000;
	double ypsnr=0.0;
	double yssim=0.0;
	int Y=width*height;
	int inc=1;
	flag=encoder.init(fps,width,height,width,height,kbps);
	if(!flag){
		perror("init encoder error");
		abort();
	}

    while(tick<totalTick){
    	if(reader.IsEnd()){
    		break;
    	}
    	bool bwUpdate=false;
    	if(reader.ShouldUpdateBandwidth(tick)){
    		bw=reader.getBandwidth();
    		reader.Update(tick);
    		bwUpdate=true;
    	}
    	if(bwUpdate){
    		kbps=bw*1000;
    		encoder.set_bitrate(kbps);
    	}
    	if(camera.getNewFrame(tick,buffer.get(),bufSize,readSize)){
    		flag=encoder.encode(buffer.get(),bufSize,AV_PIX_FMT_YUV420P,
    		encodebuffer,&out_size,&frame_type,false);
    		if(!flag){
    		perror("encoder error");
    		break;
    		}
    		if(fp_encode&&!fwrite(encodebuffer,out_size,1,fp_encode)){
    			perror("write encoded data failed");
    			break;
    		}
    		int ret=0;
    		int decode_size=0;
    		int w=0;
    		int h=0;
            ret=decoder.X264Decoder_Decode(handler,encodebuffer,out_size,decodebuffer.get()
					,&decode_size,&w,&h);
            /*if((frameCount<writeDown)&&fp_yuv&&!fwrite(decodebuffer.get(),decode_size,1,fp_yuv)){
    			perror("write decode data failed");
    			break;
    		}*/
            yssim=x264_pixel_ssim_wxh(buffer.get(), width, decodebuffer.get(), width, width, height);
            ypsnr=calculate_psnr(buffer.get(),decodebuffer.get(),inc,Y);
            if(fp_q){
            	fprintf(fp_q,"%d\t%.3f\t%.3f\n",frameCount,ypsnr,yssim);
            }
            frameCount++;
    	}
    	tick++;
    }
    if(fp_encode){
    	fclose(fp_encode);
    }
    if(fp_yuv){
    	fclose(fp_yuv);
    }
	if(fp_q){fclose(fp_q);}
    decoder.X264Decoder_UnInit(handler);
    return 0;
}
