#include <string>
#include <signal.h>
#include "nullsink.h"
#include "filecamera.h"
#include "log.h"
#include "rtc_base/timeutils.h"

using namespace rtc;
using namespace zsy;
using namespace ns3;
bool m_running=true;
int run_time=30000;
void signal_exit_handler(int sig)
{
	m_running=false;
}
int main(){
signal(SIGTERM, signal_exit_handler);
signal(SIGINT, signal_exit_handler);
signal(SIGTSTP, signal_exit_handler);
LogComponentEnable("NullSink", LOG_LEVEL_INFO);
LogComponentEnable("FileCamera", LOG_LEVEL_INFO);
std::string name=std::string("videoraw.yuv");
int frame_rate=30;
FileCamera   camera(640,480,name,frame_rate);
NullSink sink;
camera.AddOrUpdataSink(&sink);
uint32_t Stop=rtc::TimeMillis()+run_time;
camera.StartCapture();
while(m_running&&(rtc::TimeMillis()<Stop)){

}
camera.StopCapture();
return 0;
}




