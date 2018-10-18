#include <assert.h>
#include "imagerecord.h"
namespace zsy{
ImageRecord::ImageRecord(std::string name,rtc::Ns3TaskQueue *q)
:fp_(NULL),queue_(q){
	fp_=fopen(name.c_str(),"wb");
	assert(fp_);
	for(int i=0;i<10;i++){
		VideoFrame *f=new VideoFrame();
		free_buf_.push(f);
	}
	running_=true;
}
ImageRecord::~ImageRecord(){
    VideoFrame *f=NULL;
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
	if(fp_){
		fclose(fp_);
	}
}
void ImageRecord::OnEncodedImageCallBack(uint8_t *p,uint32_t len,uint32_t ts){
	VideoFrame *f=NULL;
	if(!running_){return;}
	{
		rtc::CritScope cs(&free_mutex_);
		if(free_buf_.empty()){
			f=new VideoFrame();
		}else{
			f=free_buf_.front();
			free_buf_.pop();
		}
	}
	f->fill_buf(p,len);
	f->set_time(ts);
	{
		rtc::CritScope cs(&frame_mutex_);
		frame_buf_.push(f);
	}
	if(queue_){
		queue_->PostTask([this]{
			this->Write();
		});
	}
}
void ImageRecord::Write(){
	if(!running_){return;}
	VideoFrame *f=NULL;
	{
		rtc::CritScope cs(&frame_mutex_);
		if(!frame_buf_.empty()){
			f=frame_buf_.front();
			frame_buf_.pop();
		}
	}
	if(f){
		fwrite(f->get_buf(),1,f->size(),fp_);
		{
			rtc::CritScope cs(&free_mutex_);
			free_buf_.push(f);
		}
	}
}
void ImageRecord::Stop(){
	running_=false;
	fclose(fp_);
	fp_=NULL;
}
}




