#ifndef VIDEO_IMAGERECORD_H_
#define VIDEO_IMAGERECORD_H_
#include <string>
#include <queue>
#include <stdio.h>
#include "rtc_base/task_queue.h"
#include "videocodec.h"
namespace zsy{
class ImageRecord:public EncodedVideoCallback{
public:
	ImageRecord(std::string name,rtc::Ns3TaskQueue *q);
	~ImageRecord();
	void OnEncodedImageCallBack(uint8_t *p,uint32_t len,uint32_t ts) override;
	void Write();
	void Stop();
private:
	FILE *fp_;
	rtc::Ns3TaskQueue *queue_;
	std::queue<VideoFrame*> frame_buf_;
	std::queue<VideoFrame*> free_buf_;
	rtc::CriticalSection frame_mutex_;
	rtc::CriticalSection free_mutex_;
	bool running_;
};
}



#endif /* VIDEO_IMAGERECORD_H_ */
