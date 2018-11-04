#ifndef MULTIPATH_RATECONTROL_H_
#define MULTIPATH_RATECONTROL_H_
#include "mpcommon.h"
namespace ns3{
class RateChangeCallback{
public:
	virtual ~RateChangeCallback(){}
	virtual void ChangeRate(uint32_t bitrate)=0;
};
class RateControl{
public:
	virtual ~RateControl(){}
	virtual void SetSender(SenderInterface* s)=0;
	virtual void ChangeRate(uint8_t pid,uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt)=0;
};
}




#endif /* MULTIPATH_RATECONTROL_H_ */
