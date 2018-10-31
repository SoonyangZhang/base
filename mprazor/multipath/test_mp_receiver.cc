#include "mpsession.h"
#include "cf_platform.h"
#include  <signal.h>
#include "log.h"
#include "rtc_base/timeutils.h"
#include "fakevideoconsumer.h"
#include <stdio.h>
using namespace zsy;
using namespace ns3;
bool running=true;
uint32_t run_time=4000;
void signal_exit_handler(int sig)
{
	running=false;
}
typedef struct
{
	int			msg_id;
	uint32_t	val;
}thread_msg_t;
typedef std::list<thread_msg_t>	msg_queue_t;
static msg_queue_t main_queue;
su_mutex main_mutex;
static void notify_callback(void* event, int type, uint32_t val)
{
	thread_msg_t msg;
	msg.msg_id = NOTIFYMESSAGE::notify_unknow;

	switch (type){
	case NOTIFYMESSAGE::notify_dis:
		msg.msg_id = NOTIFYMESSAGE::notify_dis;
		msg.val = val;
		break;
	case NOTIFYMESSAGE::notify_dis_ack:
		msg.msg_id = NOTIFYMESSAGE::notify_dis_ack;
		msg.val = val;
		break;
	case NOTIFYMESSAGE::notify_con:
		msg.msg_id = NOTIFYMESSAGE::notify_con;
		msg.val = val;
		break;

	default:
		return;
	}
	su_mutex_lock(main_mutex);
	main_queue.push_back(msg);
	su_mutex_unlock(main_mutex);
}
int main(){
    LogComponentEnable("MultipathReceiver", LOG_LEVEL_ALL);
    LogComponentEnable("PathInfo", LOG_LEVEL_ALL);
    //LogComponentEnable("MultipathSession", LOG_LEVEL_ALL);
    //LogComponentEnable("FakeVideoConsumer", LOG_LEVEL_ALL);
    //LogComponentEnable("MultipathSession", LOG_LEVEL_ALL);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);
	main_mutex = su_create_mutex();  
    MultipathSession session(4321,2);
    session.RegisterCallback(notify_callback,NULL);
    session.Start();
    uint32_t stop=rtc::TimeMillis()+run_time;
	thread_msg_t msg;
	FakeVideoConsumer consumer;
    while(running){
       	uint32_t now=rtc::TimeMillis();
    		su_mutex_lock(main_mutex);
    		if (main_queue.size() > 0){
    			msg = main_queue.front();
    			main_queue.pop_front();
    			su_mutex_unlock(main_mutex);
    			switch(msg.msg_id){
    			case NOTIFYMESSAGE::notify_con:{
                    printf("con requuest\n");
    				session.RegisterConsumer(&consumer);
    				break;
    			}
    			case NOTIFYMESSAGE::notify_dis:
    			case NOTIFYMESSAGE::notify_dis_ack:{
    				running=false;
    			}
    			}
    		}else{
    			su_mutex_unlock(main_mutex);
    		}
    }
    session.Stop();
    return 0;
}
