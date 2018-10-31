#include <memory.h>
#include "videoframe.h"
namespace zsy{
VideoFrame::VideoFrame()
{
	size_=0;
	capacity_=0;
	empty_=true;
}
void VideoFrame::ini(uint32_t len){
	buf_.reset(new uint8_t[len]);
	capacity_=len;
}
void VideoFrame::fill_buf(uint8_t *data,uint32_t len){
	if(len<=0){return;}
	if(len>capacity_){
		buf_.reset(new uint8_t[len]);
		capacity_=len;
	}
	memcpy(buf_.get(),data,len);
	size_=len;
	empty_=false;
}
}




