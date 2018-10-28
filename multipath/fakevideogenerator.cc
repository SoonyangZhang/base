#include "fakevideogenerator.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <string>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("FakeVideoGenerator");
FakeVideoGenerator::FakeVideoGenerator(uint32_t minbitrate,uint32_t fs)
:min_bitrate_(minbitrate)
,fs_(fs)
,rate_(min_bitrate_)
,first_ts_(0){

}
void FakeVideoGenerator::ChangeRate(uint32_t bitrate){
	if(bitrate<min_bitrate_){
		rate_=min_bitrate_;
	}
	else{
		rate_=bitrate;
	}
	uint32_t now=rtc::TimeMillis();
	if(first_ts_==0){
		first_ts_=now;
	}
	uint32_t abs_time=now-first_ts_;
	NS_LOG_INFO("time "<<std::to_string(abs_time)<<" rate "<<std::to_string(rate_));
}
}




