#include "path.h"
#include "rtc_base/timeutils.h"
namespace zsy{
PathInfo::PathInfo()
:fd(0),pid{0}
,state(path_ini)
,con_c{0}
,con_ts(0)
,rtt_(0)
,rtt_var_(5)
,rtt_update_ts_(0)
,trans_seq_(1)
,packet_seed_(1)
,rate_(0)
,base_seq_(0)
,s_sent_ts_(0)
,max_seq_(0)
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
sim_segment_t *PathInfo::get_segment(uint32_t id,int retrans){
	sim_segment_t *seg=NULL;
	if(retrans==0){
		auto it=buf_.find(id);
		if(it!=buf_.end()){
			seg=it->second;
			len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
			buf_.erase(it);
			sent_buf_.insert(std::make_pair(id,seg));
		}
	}else{
		auto it=sent_buf_.find(id);
		if(it!=sent_buf_.end()){
			seg=it->second;
		}
	}

    return seg;
}
uint32_t PathInfo::get_delay(){
	uint32_t delta=0;
	if(buf_.empty()){
	}else{
		uint32_t now=rtc::TimeMillis();
		auto it=buf_.begin();
		delta=now-it->second->timestamp;
	}
	return delta;
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
void PathInfo::ReceiverUpdateLoss(uint32_t seq,uint32_t ts){

}
void PathInfo::OnReceiveSegment(sim_segment_t *seg){
	uint32_t seq=seg->packet_id;
	if(max_seq_==0&&seg->packet_id>seg->index){
		max_seq_=seg->packet_id-seg->index-1;
		base_seq_ = seg->packet_id - seg->index - 1;
	}

	max_seq_=SU_MAX(max_seq_,seq);
	if (seq == base_seq_ + 1)
		base_seq_ = seq;
}
void PathInfo::Consume(uint32_t packet_id){

}
}




