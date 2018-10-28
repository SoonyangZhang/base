#ifndef MULTIPATH_SCHEDULE_H_
#define MULTIPATH_SCHEDULE_H_
#include "path.h"
#include <map>
namespace zsy{
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
}
#endif /* MULTIPATH_SCHEDULE_H_ */
