#ifndef VIDEO_IMAGERECORD_H_
#define VIDEO_IMAGERECORD_H_
#include <string>
#include <queue>
#include <stdio.h>
#include "task_queue.h"
#include "datasink.h"
#include "videocodec.h"
namespace zsy{
class ImageRecord:public EncodedVideoCallback{
public:
	ImageRecord(std::string &name,zsy::Ns3TaskQueue *q);
	~ImageRecord();
	void OnEncodedImageCallBack(uint8_t *p,uint32_t len,uint32_t ts) override;
	void Write();
	void Stop();
private:
	FILE *fp_;
	zsy::Ns3TaskQueue *queue_;
	std::queue<VideoFrame*> frame_buf_;
	std::queue<VideoFrame*> free_buf_;
	rtc::CriticalSection frame_mutex_;
	rtc::CriticalSection free_mutex_;
	bool running_;
};
class EndecoderBridge :public EncodedVideoCallback{
public:
	EndecoderBridge(DataSink<VideoFrame>&);
	void OnEncodedImageCallBack(uint8_t *p,uint32_t len,uint32_t ts) override;
private:
	DataSink<VideoFrame> &cb_;
	VideoFrame f_;
};
class DecodeRecord:public DecodedImageCallback{
public:
	DecodeRecord(std::string&name,zsy::Ns3TaskQueue *q);
	~DecodeRecord();
	void OnDecodedImageCallback(uint8_t*,uint32_t) override;
	void Write();
	void Stop();
private:
	FILE *fp_;
	zsy::Ns3TaskQueue *queue_;
	std::queue<VideoFrame*> frame_buf_;
	std::queue<VideoFrame*> free_buf_;
	rtc::CriticalSection frame_mutex_;
	rtc::CriticalSection free_mutex_;
	bool running_;
};
}



#endif /* VIDEO_IMAGERECORD_H_ */
