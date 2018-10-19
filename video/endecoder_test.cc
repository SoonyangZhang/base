#include <string>
#include <unistd.h>
#include <signal.h>
#include "videocodec.h"
#include "filecamera.h"
#include "imagerecord.h"
#include "filecamera.h"
#include "rtc_base/timeutils.h"
#include "rtc_base/task_queue.h"
#include "log.h"
bool m_running=true;
int run_time=100000;
using namespace ns3;
using namespace zsy;
using namespace rtc;
using namespace rtc;
void signal_exit_handler(int sig)
{
	m_running=false;
}
int main(){
	LogComponentEnable("VideoEncoder", LOG_LEVEL_ALL);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);
	rtc::Ns3TaskQueue queue;
	std::string name("decode.yuv");
	zsy::DecodeRecord decoderecord(name,&queue);
	zsy::VideoDecoder decoder(decoderecord);
	decoder.Init(640,480);

	zsy::EndecoderBridge bridge(decoder);
	zsy::VideoEncoder encoder(bridge);
	int frame_rate=30;
	encoder.StartEncoder(640,480,frame_rate);
	std::string rawvideo=std::string("videoraw.yuv");
	zsy::FileCamera camera(640,480,rawvideo,frame_rate);
	camera.AddOrUpdataSink(&encoder);
	camera.StartCapture();
	int Stop=rtc::TimeMillis()+run_time;
	while(m_running&&(rtc::TimeMillis()<Stop)){

	}
	camera.StopCapture();
	camera.RemoveSink(&encoder);
	encoder.StopEncoder();
	sleep(2);
	decoderecord.Stop();
	decoder.Stop();
	queue.Stop();
	return 0;
}



