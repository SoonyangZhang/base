#include "rtc_base/timeutils.h"
#include "videocodec.h"
#include "log.h"
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("VideoEncoder");
//When you have a member in your class that is a reference, you MUST initialize it in the constructor
VideoEncoder::VideoEncoder(EncodedVideoCallback &cb):callback_(cb){
	encoder_=new H264Encoder();
	//callback_=cb;
	for(int i=0;i<10;i++){
		VideoFrame *f=new VideoFrame();
		free_buf_.push(f);
	}
	running_=false;
	width_=height_=fs_=0;
}
VideoEncoder::~VideoEncoder(){
	
	VideoFrame *f=NULL;
    if(running_){ 
    running_=false;  
    rtc::Thread::Stop();
    }
	while(!free_buf_.empty()){
		f=free_buf_.front();
		free_buf_.pop();
		delete f;
	}
	while(!frame_buf_.empty()){
		f=frame_buf_.front();
		frame_buf_.pop();
		delete f;
	}
}
void VideoEncoder::StartEncoder(uint32_t w,uint32_t h,uint32_t fs){
	width_=w;
	height_=h;
	fs_=fs;
	bool flag=false;
	flag=encoder_->init(fs_,width_,height_,width_,height_);
	if(!flag){
		perror("init encoder error");
		abort();
	}
	if(!running_){
		running_=true;
		rtc::Thread::Start();
	}
}
void VideoEncoder::StopEncoder(){
    if(running_){ 
    running_=false;  
    rtc::Thread::Stop();
    }
}
void VideoEncoder::OnIncomingData(VideoFrame*frame){
	VideoFrame *f=NULL;
	{
		rtc::CritScope cs(&free_mutex_);
		if(free_buf_.empty()){
			f=new VideoFrame();
		}else{
			f=free_buf_.front();
			free_buf_.pop();
		}
	}
	f->fill_buf(frame->get_buf(),frame->size());
	{
		rtc::CritScope cs(&frame_mutex_);
		frame_buf_.push(f);
		NS_LOG_INFO("encoder queue size"<<frame_buf_.size());
	}
}
void VideoEncoder::Run(){
	while(running_){
		VideoFrame *f=NULL;
		int out_size=0;
		int frame_type=0;
		{
			rtc::CritScope cs(&frame_mutex_);
			if(!frame_buf_.empty()){
				f=frame_buf_.front();
				frame_buf_.pop();
			}
		}
		if(f){
			encoder_->encode(f->get_buf(),f->size()
					,AV_PIX_FMT_YUV420P,image_buf_,
					&out_size,&frame_type,false);
			uint32_t now=rtc::TimeMillis();
			callback_.OnEncodedImageCallBack(image_buf_,out_size,now);
			{
				rtc::CritScope cs(&free_mutex_);
				free_buf_.push(f);
			}
		}
	}
}
void VideoEncoder::SetRate(uint32_t r){
	encoder_->set_bitrate(r);
}
}


