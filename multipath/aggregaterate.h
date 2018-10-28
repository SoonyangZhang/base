#ifndef MULTIPATH_AGGREGATERATE_H_
#define MULTIPATH_AGGREGATERATE_H_
#include "ratecontrol.h"
#include <map>
namespace zsy{
class AggregateRate:public RateControl{
public:
	AggregateRate();
	~AggregateRate(){}
	void RegisterRateChangeCallback(RateChangeCallback*);
	void ChangeRate(uint8_t pid,uint32_t bitrate
			, uint8_t fraction_loss, uint32_t rtt) override;
private:
	std::map<uint8_t,uint32_t> path_rate_table_;
	RateChangeCallback *cb_;
};
}



#endif /* MULTIPATH_AGGREGATERATE_H_ */
