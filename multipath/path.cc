#include "path.h"
#include "rtc_base/timeutils.h"
namespace zsy{
PathInfo::PathInfo()
:fd(0),pid{0}
,state(path_ini)
,con_c{0}
,con_ts(0)
,rtt(0)
,rtt_update_ts(0)
,trans_seq(1)
,len_(0),
controller_(NULL){
}
PathInfo::~PathInfo(){
	if(controller_){
		delete controller_;
	}
}
bool PathInfo::put(sim_segment_t*seg){
	uint32_t id=seg->packet_id;
	auto it=buf_.find(id);
	if(it!=buf_.end()){
		return false;
	}
	len_+=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	buf_.insert(std::make_pair(id,seg));
	return true;
}
sim_segment_t *PathInfo::get_segment(uint32_t id){
	sim_segment_t *seg=NULL;
	auto it=buf_.find(id);
	if(it!=buf_.end()){
		seg=it->second;
		len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
		buf_.erase(it);
	}
    return seg;
}
uint32_t PathInfo::get_delay(){
	uint32_t delta=0;
	if(buf_.empty()){
		return delta;
	}else{
		uint32_t now=rtc::TimeMillis();
		auto it=buf_.begin();
		delta=now-it->second->timestamp;
		return delta;
	}
}
uint32_t PathInfo::get_len(){
	return len_;
}
void PathInfo::SetController(CongestionController *c){
	if(controller_){
		delete controller_;
	}
	controller_=c;
}
CongestionController* PathInfo::GetController(){
	return controller_;
}
}




