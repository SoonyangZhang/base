#ifndef MULTIPATH_AGGREGATERATE_H_
#define MULTIPATH_AGGREGATERATE_H_
#include "ratecontrol.h"
#include <map>
#include <iostream>
#include <fstream>
namespace ns3{
class AggregateRate:public RateControl{
public:
	AggregateRate();
	~AggregateRate();
	void RegisterRateChangeCallback(RateChangeCallback*);
	void ChangeRate(uint8_t pid,uint32_t bitrate
			, uint8_t fraction_loss, uint32_t rtt) override;
    void SetSender(SenderInterface* s) override{sender_=s;}
private:
	std::map<uint8_t,uint32_t> path_rate_table_;
	RateChangeCallback *cb_;
	SenderInterface *sender_;
    double first_ts_;
    bool first_;
    uint8_t a_pid_{10};
    std::fstream m_traceRateFile;
};
}



#endif /* MULTIPATH_AGGREGATERATE_H_ */
