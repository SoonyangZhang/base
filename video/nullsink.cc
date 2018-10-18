#include "nullsink.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <iostream>
using namespace ns3;
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("NullSink");
void NullSink::OnIncomingData(VideoFrame *f){
	uint32_t now=rtc::TimeMillis();
	if(flag_){
		first_=now;
		last_=now;
		flag_=false;
	}
	uint32_t absolute=now-first_;
	uint32_t delta=now-last_;
	last_=now;
	uint32_t size=f->size();
    //std::cout<<"as_ts "<<absolute<<" delta "<<delta<<"size "<<size<<std::endl;
	NS_LOG_INFO ("ns as_ts "<<absolute<<" delta "<<delta<<"size "<<size);
}
}



