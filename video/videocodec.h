#ifndef VIDEO_VIDEOCODEC_H_
#define VIDEO_VIDEOCODEC_H_
#include <queue>
#include <vector>
#include "rtc_base/thread.h"
#include "rtc_base/criticalsection.h"
#include "echo_h264_encoder.h"
#include "datasink.h"
#include "videoframe.h"
namespace zsy{
class EncodedVideoCallback{
public:
	virtual ~EncodedVideoCallback(){}
	virtual void OnEncodedImageCallBack(uint8_t *,uint32_t,uint32_t)=0;
};
class VideoEncoder :public DataSink<VideoFrame>,rtc::Thread{
public:
	VideoEncoder(EncodedVideoCallback &);
	~VideoEncoder();
	void StartEncoder(uint32_t w,uint32_t h,uint32_t fs);
	void StopEncoder();
	void OnIncomingData(VideoFrame*) override;
	void Run() override;
	void SetRate(uint32_t);
private:
	H264Encoder *encoder_;
	EncodedVideoCallback &callback_;
	std::queue<VideoFrame*> frame_buf_;
	std::queue<VideoFrame*> free_buf_;
	rtc::CriticalSection frame_mutex_;
	rtc::CriticalSection free_mutex_;
	bool running_;
	uint32_t width_;
	uint32_t height_;
	uint32_t fs_;
	uint8_t image_buf_[1024000];
};
}
#endif /* VIDEO_VIDEOCODEC_H_ */
