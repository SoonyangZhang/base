#include <assert.h>
#include <stdio.h>
#include <algorithm>
#include "filecamera.h"
#include "log.h"
#include <string.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("FileCamera");
FileCamera::FileCamera(int width,int height,std::string name,int rate)
:width_(width),height_(height),fs_(rate),fp_(NULL){
	fp_=fopen(name.c_str(),"rb");
	assert(fp_);
	buf_=new VideoFrame[2];
	cur_=&buf_[0];
	free_=&buf_[1];
	frame_size_=width_*height_*3/2;
	data_buf_=new uint8_t[frame_size_];
	m_running_=false;
	read_end_=false;
	ReadFrame(cur_);
	ReadFrame(free_);
}
FileCamera::~FileCamera(){
	m_running_=false;
	taskQueue_.Stop();
	delete [] buf_;
	delete data_buf_;
	if(fp_){
		fclose(fp_);
	}
}
void FileCamera::ReadFrame(VideoFrame *buf){
	int ret=0;
	if(!fp_){
		buf->Clear();
		read_end_=true;
		return ;
	}
	ret=fread(data_buf_,1,frame_size_,fp_);
    NS_LOG_INFO ("read size "<<ret);
	buf->fill_buf(data_buf_,ret);
	if(ret<frame_size_){
		buf->Clear();
		read_end_=true;
	}
}
void FileCamera::NextFrame(){
	if(read_end_){return ;}
	if(m_running_==false){return ;}
	DeliverFrame(cur_);
	VideoFrame *temp;
	temp=cur_;
	cur_=free_;
	free_=temp;
	free_->Clear();
	ReadFrame(cur_);
	int next=1000/fs_;
	if(!read_end_){
		taskQueue_.PostDelayedTask([this]{
			this->NextFrame();
		},next);
	}
}
void FileCamera::StartCapture(){
	m_running_=true;
	if(!cur_->Empty()){
		VideoFrame *temp;
		temp=cur_;
		DeliverFrame(cur_);
		cur_=free_;
		free_=temp;
		free_->Clear();
		ReadFrame(cur_);
		int next=1000/fs_;
		if(!read_end_){
			taskQueue_.PostDelayedTask([this]{
				this->NextFrame();
			},next);
		}
	}
}
void FileCamera::StopCapture(){
	m_running_=false;
}
void FileCamera::AddOrUpdataSink(DataSink<VideoFrame>*s){
	rtc::CritScope cs(&mutex_);
	if (sinks_.empty()) {
		sinks_.push_back(s);
        return ;
	}
	auto it = std::find(sinks_.begin(), sinks_.end(),s);
	if (it == sinks_.end()) {
		sinks_.push_back(s);
	}
}
void FileCamera::RemoveSink(DataSink<VideoFrame>*s){
	rtc::CritScope cs(&mutex_);
	if (sinks_.empty()) {
		return ;
	}
	for (auto itor = sinks_.begin(); itor != sinks_.end();)
	{
		//infact not to iter all the elements
		if (*itor == s)
		{
			itor = sinks_.erase(itor);
		}else{
			itor++;
		}
	}
}
void FileCamera::DeliverFrame(VideoFrame* f){
	rtc::CritScope cs(&mutex_);
    for(auto itor=sinks_.begin();itor!=sinks_.end();itor++){
    (*itor)->OnIncomingData(f);
    }
}
}



