#include "rtc_base/timeutils.h"
#include "rtc_base/location.h"
#include "modules/include/module.h"
#include "modules/utility/source/process_thread_impl.h"
#include <signal.h>
class MyModule:public webrtc::Module{
public:
	 int64_t TimeUntilNextProcess() override;
	 void Process() override;
private:
	uint32_t last_time_{0};
	uint32_t c_{0};
	int64_t gap_{10};
};
int64_t MyModule::TimeUntilNextProcess(){
	if(last_time_==0){
	return 0;
	}else{
		return gap_;
	}
}
void MyModule::Process(){
	uint32_t now=rtc::TimeMillis();
	last_time_=now;
	c_++;
	printf("%d\n",c_);
}
static bool running=true;
void signal_exit_handler(int sig)
{
	running=false;
}
uint32_t run_time=100;
int main(){
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);
	std::string thread_name=std::string("test");
	webrtc::ProcessThreadImpl thread(thread_name.c_str());
	MyModule module;
    std::string  fileAndLine;
	fileAndLine=std::string("webrtcsender")+std::to_string(__LINE__);
	rtc::Location location(__FUNCTION__,fileAndLine.c_str());
	thread.RegisterModule(&module,location);
	thread.Start();
	uint32_t Stop=rtc::TimeMillis()+run_time;
	while(running&&(rtc::TimeMillis()<Stop)){

	}
	thread.Stop();
	return 0;
}




