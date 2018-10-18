#include <iostream>
#include<signal.h>
#include <string>
#include "rtc_base/task_queue.h"
#include "rtc_base/timeutils.h"
#include "rate_record.h"
zsy::RateRecord *g_r;
static bool m_running=true;
static uint32_t run_time=100;
void signal_exit_handler(int sig)
{
	m_running=false;
}

int main(){
	signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGTSTP, signal_exit_handler);
    std::string name=std::string("record.txt");
    rtc::Ns3TaskQueue q;
    zsy::RateRecord r(&q,name);
    g_r=&r;
    r.ChangeBitrate(100);
    r.ChangeBitrate(300);
    uint32_t Stop=rtc::TimeMillis()+run_time;
    uint32_t now=0;
    while(m_running){
    	now=rtc::TimeMillis();
    	if(now>Stop){
    		break;
    	}
    }
    g_r=NULL;
    r.Stop();
    q.Stop();
}
