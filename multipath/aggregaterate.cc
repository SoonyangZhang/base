#include "aggregaterate.h"
#include "rtc_base/timeutils.h"
namespace zsy{
AggregateRate::AggregateRate()
:cb_(NULL)
,first_ts_(0){

}
void AggregateRate::ChangeRate(uint8_t pid,uint32_t bitrate
		, uint8_t fraction_loss, uint32_t rtt){
	auto it=path_rate_table_.find(pid);
    uint32_t now=rtc::TimeMillis();
    if(first_ts_==0){ 
        first_ts_=now;
    }
    uint32_t delta=now-first_ts_;
	if(it==path_rate_table_.end()){
		path_rate_table_.insert(std::make_pair(pid,bitrate));
	}else{
		it->second=bitrate;
	}
	uint32_t totalrate=0;
	for(auto it=path_rate_table_.begin();it!=path_rate_table_.end();it++){
		totalrate+=it->second;
	}
	if(cb_){
		cb_->ChangeRate(totalrate);
	}
    printf("%d %d %d\n",pid,delta,bitrate/1000);
    printf("%d %d %d\n",a_pid_,delta,totalrate/1000);
}
void AggregateRate::RegisterRateChangeCallback(RateChangeCallback*cb){
	cb_=cb;
}
}




