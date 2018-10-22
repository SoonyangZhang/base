#include "mpsession.h"
#include "cf_platform.h"
#include  <signal.h>
#include "log.h"
#include "rtc_base/timeutils.h"
using namespace zsy;
using namespace ns3;
bool running=true;
uint32_t run_time=4000;
void signal_exit_handler(int sig)
{
	running=false;
}
int main(){
    LogComponentEnable("MultipathReceiver", LOG_LEVEL_ALL);
    //LogComponentEnable("MultipathSession", LOG_LEVEL_ALL);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);  
    MultipathSession receiver(4321,2);
    receiver.Start();
    uint32_t stop=rtc::TimeMillis()+run_time;
    while(running){}
    receiver.Stop();
    return 0;
}
