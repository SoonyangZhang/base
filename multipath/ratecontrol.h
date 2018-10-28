#ifndef MULTIPATH_RATECONTROL_H_
#define MULTIPATH_RATECONTROL_H_
#include "mpcommon.h"
namespace zsy{
class RateChangeCallback{
public:
	virtual ~RateChangeCallback(){}
	virtual void ChangeRate(uint32_t bitrate)=0;
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




#endif /* MULTIPATH_RATECONTROL_H_ */
