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
    LogComponentEnable("MultipathSender", LOG_LEVEL_ALL);
    LogComponentEnable("MultipathSession", LOG_LEVEL_ALL);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);
    MultipathSession sender(1234,1);
    su_addr local;
    su_addr remote;
    su_set_addr(&local,"192.168.148.136",1234);
    su_set_addr(&remote,"123.456.789.123",4321);
    su_addr remote2;
    su_set_addr(&remote2,"192.168.148.136",4321);
    sender.Connect(4,local,remote,local,remote2);
    sender.Start();
    uint32_t stop=rtc::TimeMillis()+run_time;
    while(running){}
    sender.Stop();
    return 0;
}
