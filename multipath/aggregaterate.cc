#include "aggregaterate.h"
namespace zsy{
AggregateRate::AggregateRate()
:cb_(NULL){

}
void AggregateRate::ChangeRate(uint8_t pid,uint32_t bitrate
		, uint8_t fraction_loss, uint32_t rtt){
	auto it=path_rate_table_.find(pid);
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
}
void AggregateRate::RegisterRateChangeCallback(RateChangeCallback*cb){
	cb_=cb;
}
}




