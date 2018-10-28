#ifndef SIM_TRANSPORT_MPCOMMON_H_
#define SIM_TRANSPORT_MPCOMMON_H_
#include "cf_platform.h"
#include "sim_internal.h"
namespace zsy{
class PathInfo;
enum PathState{
	path_ini,
	path_conning,
	path_conned,
	path_discon,
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
	virtual void SendSegmentAck(uint8_t,sim_segment_ack_t *)=0;
};
class SenderInterface{
public:
	virtual ~SenderInterface(){}
	virtual int64_t GetFirstTs()=0;
	virtual void Reclaim(sim_segment_t *seg)=0;
	virtual PathInfo* GetPathInfo(uint8_t)=0;
	virtual void PacketSchedule(uint32_t,uint8_t)=0;
};
class SessionInterface{
public:
	~SessionInterface(){}
	virtual bool Fd2Addr(su_socket,su_addr *)=0;
	virtual void PathStateForward(int type,int value)=0;
};
}
#endif /* SIM_TRANSPORT_MPCOMMON_H_ */
