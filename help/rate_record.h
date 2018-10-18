#ifndef __RATE_RECORD_H_
#define __RATE_RECORD_H_
#include <string>
#include <stdint.h>
#include <stdio.h>
#include "rtc_base/task_queue.h"

namespace zsy{
class RateRecord{
public:
	RateRecord(rtc::Ns3TaskQueue *queue,std::string f);
	~RateRecord();
	void Stop();
	void ChangeBitrate(uint32_t rate);
private:
	rtc::Ns3TaskQueue *q_;
	bool first_flag_;
	uint32_t last_;
	FILE *fp_;
	bool running_;
};
}
#endif
