#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <iostream>
#include "psnr.h"
#include "cmdline.h"
using namespace std;
/*
../build/t_psnr -w 1280 -h 720 -s ../data/mobcal.yuv -d 4.yuv -f 420 -o result.txt    
*/
int main(int argc, char **argv)
{
    cmdline::parser a;
    a.add<int>("width", 'w', "frame width", true,1280, cmdline::range(1, 65535));
    a.add<int>("height", 'h', "frame height", true,720, cmdline::range(1, 65535));
    a.add<string>("src", 's', "input yuv", true, "xx.yuv");
    a.add<string>("dst", 'd', "decode yuv", true, "yy.yuv");
    a.add<string>("format", 'f', "yuc format 420 or 422", true, "420");
    a.add<string>("output", 'o', "psnr result", false, "result.txt");
    a.parse_check(argc, argv);
	int x=a.get<int>("width");
    int y=a.get<int>("height");
    std::string origin_yuv=a.get<string>("src");
    std::string decode_yuv=a.get<string>("dst");
    std::string format=a.get<string>("format");
    std::string result=a.get<string>("output");
    FILE *f1=nullptr, *f2=nullptr,*f3=nullptr;
    int yuv,inc = 1, Y, F;
    double ypsnr = 0.0,yssim=0.0;
    unsigned char *b1=nullptr, *b2=nullptr;
    int frameCount=0;
    if ((f1 = fopen(origin_yuv.c_str(), "rb")) == 0){
        std::cout<<"src not exist"<<std::endl;
        return 0;
    }
    if ((f2 = fopen(decode_yuv.c_str(), "rb")) == 0){
        std::cout<<"dst not exist"<<std::endl;
        return 0;        
    }
    if((f3= fopen(result.c_str(), "w+")) == 0){
        std::cout<<"result failed"<<std::endl;
        return 0;        
    }
    if ((yuv = strtoul(format.c_str(), 0, 10)) > 444){
        return 0;
    }
    //std::cout<<"yuv format "<<yuv<<std::endl;
    Y = x * y;
    switch (yuv) {
        case 400: F = Y; break;
        case 422: F = Y * 2; break;
        case 444: F = Y * 3; break;
        default :
        case 420: F = Y * 3 / 2; break;
    }

    if (!(b1 =(unsigned char*)malloc(F))){return 0;}
    if (!(b2 =(unsigned char*)malloc(F))) {return 0;}

  for (;;) {
    if (1 != fread(b1, F, 1, f1) || 1 != fread(b2, F, 1, f2)) break;
    yssim=x264_pixel_ssim_wxh(b1, x, b2, x, x, y);
    ypsnr=calculate_psnr(b1,b2,inc,inc == 1 ? Y : F);
    fprintf(f3,"%d\t%.3f\t%.3f\n",frameCount,ypsnr,yssim);
    frameCount++;
  }
  if(b1){
      free(b1);
  }
  if(b2){
      free(b2);
  }
  fclose(f1);
  fclose(f2);
  fclose(f3);
  return 0;
}
