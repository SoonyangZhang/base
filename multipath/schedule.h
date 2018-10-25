#ifndef MULTIPATH_SCHEDULE_H_
#define MULTIPATH_SCHEDULE_H_
#include "path.h"
#include <map>
namespace zsy{
class SenderInterface{
public:
	virtual ~SenderInterface(){}
	virtual int64_t GetFirstTs()=0;
	virtual void Reclaim(sim_segment_t *seg)=0;
	virtual PathInfo* GetPathInfo(uint8_t)=0;
	virtual void PacketSchedule(uint32_t,uint8_t)=0;
};
class Schedule{
public:
	virtual ~Schedule(){}
	void SetSender(SenderInterface* s){sender_=s;}
	//std::map<uint32_t,uint32_t>&packets  packet id ->length
	virtual void IncomingPackets(std::map<uint32_t,uint32_t>&packets)=0;
	virtual void RetransPackets(std::map<uint32_t,uint32_t>&packets)=0;
	virtual void RegisterPath(uint8_t)=0;
	virtual void UnregisterPath(uint8_t)=0;
protected:
	SenderInterface *sender_;
};
class RateControl{
public:
	virtual ~RateControl(){}
	void SetSender(SenderInterface* s){sender_=s;}
	virtual void ChangeRate(uint8_t pid,uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt);
protected:
	SenderInterface *sender_;
};
}
#endif /* MULTIPATH_SCHEDULE_H_ */
