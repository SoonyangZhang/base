extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
};
#include <iostream>
static int width=640;
static int height=480;

int main(){
    int buf_size=0;
    buf_size=avpicture_get_size(AV_PIX_FMT_YUV420P,width,height);
    std::cout<<"buf "<<buf_size<<std::endl;
}
