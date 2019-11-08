#include <sys/stat.h>
#include "camera.h"
#include "logging.h"
Camera::Camera(int width,int height,
		int fps,std::string &name){
    struct stat statbuf;
    stat(name.c_str(),&statbuf);
    int totalSize=statbuf.st_size;
    frame_size_=width*height*3/2;
    frames_=totalSize/frame_size_;
    frame_gap_=1000/fps;
    if(frames_<1){
    	LOG(INFO)<<"yuv no data";
    	exit(0);
    }
    LOG(INFO)<<"frames "<<frames_;
    yuv_=fopen(name.c_str(), "rb");
    CHECK(yuv_);
    random_.seedTime();
}
Camera::~Camera(){
	if(yuv_){
		fclose(yuv_);
	}
}
bool Camera::getNewFrame(int now,uint8_t *buffer,
		int bufSize,int &outSize){
	bool ret=false;
	outSize=0;
	int read=0;
	if(nextFrametick_==0){
		read=FillBuffer(buffer,bufSize);
		if(read==bufSize){
			outSize=bufSize;
			frame_count_++;
			ret=true;
		}
		nextFrametick_=now+frame_gap_;
	}else{
		if(now>=nextFrametick_){
			nextFrametick_=now+frame_gap_;
			read=FillBuffer(buffer,bufSize);
			if(read==bufSize){
				frame_count_++;
				outSize=bufSize;
				ret=true;
			}
		}
	}
	return ret;
}
int Camera::FillBuffer(uint8_t *buffer,int bufSize){
	int ret=0;
	if(feof(yuv_)){
		Reset();
	}
	ret=fread(buffer,1,bufSize,yuv_);
	if(ret!=bufSize){
		ret=0;
	}
	return ret;
}
void Camera::Reset(){
	if(!feof(yuv_)){
		LOG(INFO)<<"not end";
		return ;
	}
	int  result;
	int line=random_.nextInt(0,frames_);
	int offset=frame_size_*line;
	result=fseek(yuv_,offset,SEEK_SET);
	if(result){
		LOG(INFO)<<"reset failed";
	}else{
		LOG(INFO)<<"reset "<<line;
	}
}
