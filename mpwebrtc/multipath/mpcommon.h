#ifndef SIM_TRANSPORT_MPCOMMON_H_
#define SIM_TRANSPORT_MPCOMMON_H_
#include "cf_platform.h"
#include "sim_proto.h"
#include <stdint.h>
namespace zsy{
#define MIN_BITRATE		80000					/*10KB*/
#define MAX_BITRATE		16000000				/*2MB*/
#define START_BITRATE	800000					/*100KB*/
typedef void(*sim_notify_fn)(void* event, int type, uint32_t val);
class PathSender;
enum PathState{
	path_ini,
	path_conning,
	path_conned,
	path_discon,
};
enum{
	gcc_transport = 0,
	bbr_transport = 1,
};
enum NOTIFYMESSAGE{
	notify_unknow,
	notify_con_ack,
	notify_con,
    notify_dis,
	notify_dis_ack,
};
//to consume frame
class NetworkDataConsumer{
public:
	virtual ~NetworkDataConsumer(){}
	virtual void ForwardUp(uint32_t fid,uint8_t*data,uint32_t len,uint32_t recv,uint32_t total)=0;
};
class ReceiverInterface{
public:
	virtual ~ReceiverInterface(){}
    virtual uint32_t GetUid()=0;
    virtual void DeliverToCache(uint8_t pid,sim_segment_t* d)=0;
};
class SenderInterface{
public:
	virtual ~SenderInterface(){}
	virtual int64_t GetFirstTs()=0;
	virtual uint32_t GetUid()=0;
	virtual void Reclaim(sim_segment_t *seg)=0;
	virtual PathSender* GetPathInfo(uint8_t)=0;
	virtual void PacketSchedule(uint32_t,uint8_t)=0;
	virtual void OnNetworkChanged(uint8_t pid,
								  uint32_t bitrate_bps,
								  uint8_t fraction_loss,
								  int64_t rtt_ms)=0;
};
class SessionInterface{
public:
	virtual ~SessionInterface(){}
	virtual void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t len)=0;
	virtual bool Fd2Addr(su_socket,su_addr *)=0;
	virtual void PathStateForward(int type,int value)=0;
};
}
#endif /* SIM_TRANSPORT_MPCOMMON_H_ */
