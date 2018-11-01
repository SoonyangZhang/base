#include <list>
#include "mpsession.h"
#include "aggregaterate.h"
#include "randomschedule.h"
#include "cf_platform.h"
#include  <signal.h>
#include "log.h"
#include "rtc_base/timeutils.h"
#include "fakevideogenerator.h"
using namespace zsy;
using namespace ns3;
bool running=true;
uint32_t run_time=200000;
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
	case NOTIFYMESSAGE::notify_con_ack:
		msg.msg_id = NOTIFYMESSAGE::notify_con_ack;
		msg.val = val;
		break;

	default:
		return;
	}

	su_mutex_lock(main_mutex);
	main_queue.push_back(msg);
	su_mutex_unlock(main_mutex);
}
#define MAX_SEND_BITRATE (300 * 8 * 1000)
#define MIN_SEND_BITRATE (20 * 8 * 1000)
#define START_SEND_BITRATE (140 * 8 * 1000)
int main(){
    LogComponentEnable("MultipathSender", LOG_LEVEL_ALL);
    LogComponentEnable("PathInfo", LOG_LEVEL_ALL);
    //LogComponentEnable("MultipathSession", LOG_LEVEL_ALL);
    //LogComponentEnable("FakeVideoGenerator", LOG_LEVEL_ALL);
	signal(SIGTERM, signal_exit_handler);
	signal(SIGINT, signal_exit_handler);
	signal(SIGTSTP, signal_exit_handler);
	main_mutex = su_create_mutex();
	FakeVideoGenerator generator(MIN_SEND_BITRATE,30);
	AggregateRate rate;
	rate.RegisterRateChangeCallback(&generator);
	RandomSchedule schedule;
    MultipathSession session(1234,1);
    generator.RegisterSession(&session);
    session.RegistePacketSchedule(&schedule);
    session.RegisterRateControl(&rate);
    session.RegisterCallback(notify_callback,NULL);
    su_addr local1;
    su_addr remote1;
    su_addr local2;
    su_addr remote2;
    su_set_addr(&local1,"10.0.1.1",1234);
    su_set_addr(&remote1,"10.0.2.2",4321);
    su_set_addr(&local2,"10.0.3.1",1234);
    su_set_addr(&remote2,"10.0.4.2",4321);
    session.Connect(2,local1,remote1);//,local2,remote2);
    session.Start();
    uint32_t stop=rtc::TimeMillis()+run_time;
    bool can_send_video=false;
    bool send_dis_con=false;
	thread_msg_t msg;
    while(running){
    	uint32_t now=rtc::TimeMillis();
		su_mutex_lock(main_mutex);
		if (main_queue.size() > 0){
			msg = main_queue.front();
			main_queue.pop_front();
			su_mutex_unlock(main_mutex);
			switch(msg.msg_id){
			case NOTIFYMESSAGE::notify_con_ack:{
				can_send_video=true;
				break;
			}
			case NOTIFYMESSAGE::notify_dis:
			case NOTIFYMESSAGE::notify_dis_ack:
			{
				can_send_video=false;
				running=false;
				break;
			}
			}
		}else{
			su_mutex_unlock(main_mutex);
		}
		if(can_send_video){
			generator.HeartBeat(now);
		}
    	if((now>stop)&&!send_dis_con){
            printf("send discon\n");
            SessionMessage s_msg;
            s_msg.type=MessageType::message_send_dis;
    		session.PostMessage(s_msg);
    		generator.Stop();
    		send_dis_con=true;
    	}
    }
    session.Stop();
    return 0;
}
