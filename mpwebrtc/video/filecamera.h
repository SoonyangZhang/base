// read yuv data from file 
// a fake camera
#ifndef __FILE_CAMERA_H_
#define __FILE_CAMERA_H_
#include <string>
#include <vector>
#include "task_queue.h"
#include "rtc_base/criticalsection.h"
#include "videoframe.h"
#include "datasink.h"
namespace zsy{
class FileCamera{
public:
	FileCamera(int width,int height,std::string name,int rate);
	~FileCamera();
	void StartCapture();
	void StopCapture();
	void AddOrUpdataSink(DataSink<VideoFrame>*s);
	void RemoveSink(DataSink<VideoFrame>*s);
	void NextFrame();
private:
	void DeliverFrame(VideoFrame*);
	void ReadFrame(VideoFrame *buf);
	int width_;
	int height_;
	int fs_;
	FILE *fp_;
	VideoFrame *buf_;
	VideoFrame *cur_;
	VideoFrame *free_;
	int frame_size_;
	uint8_t *data_buf_;
	bool m_running_;
	bool read_end_;
	rtc::CriticalSection mutex_;
	std::vector<DataSink<VideoFrame>*>sinks_;
	zsy::Ns3TaskQueue taskQueue_;

};
}
#endif
