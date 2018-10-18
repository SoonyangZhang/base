/*-
* Copyright (c) 2017-2018 wenba, Inc.
*	All rights reserved.
*
* See the file LICENSE for redistribution information.
*/

/**************************************************************************
* sim_sender_test��һ��ģ����Ƶ���ݷ��͵Ĺ��̣����ڵ����շ�˫�˵���ȷ��,���ǽ���
* ��sim_transport֮�ϵģ�������������razor��sim_transport����
**************************************************************************/
#include <list>
#include "cf_platform.h"
#include "sim_external.h"
//#include "audio_log.h"

#include <time.h>
#include <assert.h>
#include<signal.h>
#include <string>
#include <unistd.h>
#include "rtc_base/task_queue.h"
#include "rtc_base/timeutils.h"
#include "rate_record.h"
zsy::RateRecord *g_r=NULL;
enum
{
	el_change_bitrate,
	el_pause,
	el_resume,

	el_connect,
	el_disconnect,
	el_timeout,

	el_unknown = 1000
};

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
	msg.msg_id = el_unknown;

	switch (type){
	case sim_connect_notify:
		msg.msg_id = el_connect;
		msg.val = val;
		break;

	case sim_network_timout:
		msg.msg_id = el_timeout;
		msg.val = val;
		break;

	case sim_disconnect_notify:
		msg.msg_id = el_disconnect;
		msg.val = val;
		break;

	case net_interrupt_notify:
		msg.msg_id = el_pause;
		msg.val = val;
		break;

	case net_recover_notify:
		msg.msg_id = el_resume;
		msg.val = val;
		break;

	default:
		return;
	}

	su_mutex_lock(main_mutex);
	main_queue.push_back(msg);
	su_mutex_unlock(main_mutex);
}

static void notify_change_bitrate(void* event, uint32_t bitrate_kbps)
{
	thread_msg_t msg;
	msg.msg_id = el_change_bitrate;
	msg.val = bitrate_kbps;
	su_mutex_lock(main_mutex);
	main_queue.push_back(msg);
	su_mutex_unlock(main_mutex);
   //printf("bit %d kbps\n",bitrate_kbps);
	if(g_r){
		g_r->ChangeBitrate(bitrate_kbps);
	}
}

static char g_info[1024] = { 0 };

static void notify_state(void* event, const char* info)
{
	strcpy(g_info, info);
}

#define MAX_SEND_BITRATE (300 * 8 * 1000)
#define MIN_SEND_BITRATE (20 * 8 * 1000)
#define START_SEND_BITRATE (140 * 8 * 1000)

typedef struct
{
	uint32_t	bitrate_kbps;	/*��ǰ���͵����ʣ�kbps*/
	int			record_flag;	/*�Ƿ���Կ�ʼ¼�Ʒ���*/
	uint32_t	frame_rate;		/*֡��*/
	int64_t		prev_ts;		/*��һ�η�����Ƶ��ʱ��*/
	int64_t		hb_ts;

	uint32_t	total_bytes;
	int			index;
	uint8_t*	frame;

}video_sender_t;

static void try_send_video(video_sender_t* sender)
{
	uint8_t* pos = sender->frame, ftype;
	int64_t now_ts, space;
	size_t frame_size = 0;
	if (sender->record_flag == 0)
		return;

	now_ts = GET_SYS_MS();
	if (now_ts >= sender->prev_ts + 1000 / sender->frame_rate){
		space = (now_ts - sender->prev_ts);
		frame_size = sender->bitrate_kbps / 8 * space;

		sender->prev_ts = now_ts;

		if (frame_size > 200){
			frame_size -= 200;
			frame_size = frame_size + rand() % 400;
		}

		memcpy(pos, &frame_size, sizeof(frame_size));
		pos += sizeof(frame_size);
		memcpy(pos, &sender->index, sizeof(sender->index));
		pos += sizeof(sender->index);
		memcpy(pos, &now_ts, sizeof(now_ts));
		pos += sizeof(now_ts);

		/*������ģ����Ĵ�С*/
		if (frame_size > 800 * 1000)
			return;

		ftype = 0;
		if (sender->index % (sender->frame_rate * 4) == 0) /*�ؼ�֡*/
			ftype = 1;

		sim_send_video(0, ftype, sender->frame, frame_size);
		/*ֻ����һ֡��һ��*/
		if (++sender->index > 20000)
			sender->record_flag = 0;
	}
}
static int run=1;
uint32_t run_time=110*1000;
void signal_exit_handler(int sig)
{
	run=0;
}
#define FRAME_SIZE (1024 * 1024)
static void main_loop_event()
{
	video_sender_t sender = {0};

	thread_msg_t msg;
	int disconnecting = 0;
	sender.frame = (uint8_t*)malloc(FRAME_SIZE);
	int64_t prev_ts, now_ts;

	prev_ts = now_ts = rtc::TimeMillis();
    uint32_t Stop=now_ts+run_time;
	while (run&&(rtc::TimeMillis()<Stop)){
		su_mutex_lock(main_mutex);
		if (main_queue.size() > 0){
			msg = main_queue.front();
			main_queue.pop_front();
			
			su_mutex_unlock(main_mutex);

			switch (msg.msg_id){
			case el_connect:
				if (msg.val == 0){
					printf("connect success!\n");
					sender.record_flag = 1;
					sender.total_bytes = 0;
					sender.frame_rate = 16;
					sender.hb_ts = sender.prev_ts = GET_SYS_MS();
					sender.bitrate_kbps = START_SEND_BITRATE / 1000;
				}
				else{
					printf("connect failed, result = %u!\n", msg.val);
					run = 0;
					sender.record_flag = 0;
				}
				break;

			case el_timeout:
				printf("network timeout!\n");
				run = 0;
				sender.record_flag = 0;
				break;

			case el_disconnect:
				printf("connect failed!\n");
				run = 0;
				sender.record_flag = 0;
				break;

			case el_pause:
				printf("pause sender!\n");
				sender.record_flag = 0;
				break;

			case el_resume:
				printf("resume sender!\n");
				sender.record_flag = 1;
				break;

			case el_change_bitrate:
				if (msg.val <= MAX_SEND_BITRATE / 1000){
					sender.bitrate_kbps = msg.val;
					printf("set bytes rate = %ukB/s\n", sender.bitrate_kbps / 8);
				}
				break;
			}
		}
		else{
			su_mutex_unlock(main_mutex);
		}

		try_send_video(&sender);

		now_ts = GET_SYS_MS();
		if (now_ts >= 1000 + prev_ts){
			printf("%s, frame id = %d\n", g_info, sender.index);
			prev_ts = now_ts;
		}

		su_sleep(0, 10000);
		if (sender.index >= 20000 && disconnecting == 0){
			sim_disconnect();
			disconnecting = 1;
		}
	}

	free(sender.frame);
}

int main(int argc, const char* argv[])
{
	signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGTSTP, signal_exit_handler);
    std::string name=std::string("record.txt");
    rtc::Ns3TaskQueue q;
    zsy::RateRecord r(&q,name);
    g_r=&r;
	srand((uint32_t)time(NULL));
	/*
	if (open_win_log("sender.log") != 0){
		assert(0);
		return -1;
	}
*/
	main_mutex = su_create_mutex();

	sim_init(1600, NULL, NULL, notify_callback, notify_change_bitrate, notify_state);
	sim_set_bitrates(MIN_SEND_BITRATE, START_SEND_BITRATE, MAX_SEND_BITRATE * 5/4);
//gcc_transport  bbr_transport
	if (sim_connect(1000, "10.0.4.2", 1600, bbr_transport,1) != 0){
		printf("sim connect failed!\n");
		goto err;
	}

	main_loop_event();

err:
	sim_destroy();
	su_destroy_mutex(main_mutex);
	g_r=NULL;
    sleep(5);
	r.Stop();
	q.Stop();

	return 0;
}




