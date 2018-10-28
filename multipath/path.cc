#include "path.h"
#include "sim_proto.h"
#include "rtc_base/timeutils.h"
namespace zsy{
PathInfo::PathInfo()
:fd(0),pid{0}
,state(path_ini)
,con_c{0}
,con_ts(0)
,rtt_(0)
,min_rtt_(0)
,rtt_var_(5)
,rtt_update_ts_(0)
,ping_resend_(0)
,trans_seq_(1)
,packet_seed_(1)
,rate_(0)
,base_seq_(0)
,s_sent_ts_(0)
,max_seq_(0)
,ack_ts_(0)
,len_(0),
controller_(NULL)
,sent_buf_c_(0)
,mpreceiver_(NULL)
,mpsender_(NULL)
,receiver_last_heart_beat_(0)
,sender_last_heart_beat_(0){
}
PathInfo::~PathInfo(){
	if(controller_){
		delete controller_;
	}
	send_buf_t *buf=NULL;
	if(!free_buf_.empty()){
		buf=free_buf_.front();
		free_buf_.pop();
		delete buf;
	}
}
void PathInfo::SenderHeartBeat(uint32_t ts){
	if(sender_last_heart_beat_+5<ts){
		razor_sender_t *cc=NULL;
		cc=controller_->s_cc_;
		if(cc){
			cc->heartbeat(cc);
		}
		sender_last_heart_beat_=ts;
	}
}
void PathInfo::ReceiverHeartBeat(uint32_t ts){
	if(receiver_last_heart_beat_+5<ts){
		VideoRealAck(1,0);
		razor_receiver_t *cc=NULL;
		cc=controller_->r_cc_;
		if(cc){
			cc->heartbeat(cc);
		}
		receiver_last_heart_beat_=ts;
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
send_buf_t * PathInfo::AllocateSentBuf(){
	send_buf_t *buf=NULL;
	if(!free_buf_.empty()){
		buf=free_buf_.front();
		free_buf_.pop();
	}else{
		buf=new send_buf_t();
	}
	return buf;
}
void PathInfo::FreePendingBuf(){
	sim_segment_t *seg=NULL;
	for(auto it=buf_.begin();it!=buf_.end();){
		seg=it->second;
		buf_.erase(it++);
		len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	    mpsender_->Reclaim(seg);
	}
}
void PathInfo::FreeSentBuf(){
	send_buf_t *buf_ptr=NULL;
	sim_segment_t *seg=NULL;
	while(!sent_buf_.empty()){
		buf_ptr=sent_buf_.front();
		sent_buf_.pop_front();
		seg=buf_ptr->seg;
		buf_ptr->seg=NULL;
		mpsender_->Reclaim(seg);
		free_buf_.push(buf_ptr);
	}
}
sim_segment_t *PathInfo::get_segment(uint32_t id,int retrans,uint32_t ts){
	sim_segment_t *seg=NULL;
	if(retrans==0){
		auto it=buf_.find(id);
		if(it!=buf_.end()){
			seg=it->second;
			len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
			buf_.erase(it);
			send_buf_t *send_buf=AllocateSentBuf();
			send_buf->ts=ts;
			send_buf->seg=seg;
			sent_buf_.push_back(send_buf);
		}
	}else{
		send_buf_t *buf=NULL;
		for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
			send_buf_t *ptr=(*it);
			if(ptr->seg->packet_id==id){
				buf=ptr;
				it=sent_buf_.erase(it);
				break;
			}else{
				it++;
			}
		}
		if(buf){
			seg=buf->seg;
			buf->ts=ts;
			sent_buf_.push_back(buf);
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
#define PATH_ACK_REAL_TIME	20
#define PATH_ACK_HB_TIME		200
void PathInfo::VideoRealAck(int hb,uint32_t seq){
	uint32_t now=rtc::TimeMillis();
	sim_segment_ack_t ack;
	if((hb==0&&(ack_ts_+PATH_ACK_REAL_TIME)<now)||(ack_ts_+PATH_ACK_HB_TIME)<now){
		ack.base_packet_id = base_seq_;
		ack.nack_num = 0;
		ack.acked_packet_id = seq;
		for(auto it=loss_.begin();it!=loss_.end();it++){
			uint32_t key=(*it);
			if(key<base_seq_){
				continue;
			}
			if(ack.nack_num<NACK_NUM){
				uint32_t nack=key-base_seq_;
				ack.nack[ack.nack_num]=nack;
				ack.nack_num++;
			}else{
				break;
			}
		}
		SendSegmentAck(&ack);
		ack_ts_=now;
	}
}
send_buf_t * PathInfo::GetSentPacketInfo(uint32_t seq){
	send_buf_t *sent_buf=NULL;
	send_buf_t *temp=NULL;
	for(auto it=sent_buf_.begin();it!=sent_buf_.end();it++){
		temp=(*it);
		if(temp->seg->packet_id==seq){
			sent_buf=temp;
			break;
		}
	}
	return sent_buf;
}
//TODO
// rtt to avoid excessive retrans
// may record minrtt;
void PathInfo::RecvSegAck(sim_segment_ack_t* ack){
	if (ack->acked_packet_id >packet_seed_ || ack->base_packet_id > packet_seed_)
		return;
	uint32_t now=rtc::TimeMillis();
	send_buf_t *temp=NULL;
	temp=GetSentPacketInfo(ack->acked_packet_id);
	if(temp){
		uint32_t rtt=now-temp->ts;
		UpdateRtt(rtt);
	}
	SenderUpdateBase(ack->base_packet_id);
	RemoveAckedPacket(ack->acked_packet_id);
	uint32_t minrtt=min_rtt_;
	uint32_t i=0;
	sim_segment_t *seg=NULL;
	razor_sender_t *cc=NULL;
	cc=controller_->s_cc_;
	for(i=0;i<ack->nack_num;i++){
		uint32_t seq=ack->base_packet_id+ack->nack[i];
		temp=GetSentPacketInfo(seq);
		if(temp){
			if((now-temp->ts)>minrtt&&cc){
				temp->ts=now;
				seg=temp->seg;
				cc->add_packet(cc,seg->packet_id, 1, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
			}
		}
	}

}
void PathInfo::SenderUpdateBase(uint32_t base_id){
	uint32_t i=0;
	for(i=base_seq_;i<=base_id;i++){
		RemoveAckedPacket(i);
	}
	if(base_seq_<base_id){
		base_seq_=base_id;
	}
}
void PathInfo::RemoveAckedPacket(uint32_t seq){
	send_buf_t *buf_ptr=NULL;
	for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
		if((*it)->seg->packet_id==seq){
			buf_ptr=(*it);
			sent_buf_.erase(it++);
		}else{
			it++;
		}
	}
	if(buf_ptr){
		mpsender_->Reclaim(buf_ptr->seg);
		buf_ptr->seg=NULL;
		free_buf_.push(buf_ptr);
	}
}
void PathInfo::SendSegmentAck(sim_segment_ack_t *ack){
	if(mpreceiver_){
		mpreceiver_->SendSegmentAck(pid,ack);
	}
}
void PathInfo::UpdateRtt(uint32_t time){
	uint32_t keep_rtt=5;
	if(time>keep_rtt){
		keep_rtt=time;
	}
	UpdateMinRtt(keep_rtt);
	rtt_var_ = (rtt_var_ * 3 + SU_ABS(rtt_, keep_rtt)) / 4;
	if (rtt_var_ < 10)
		rtt_var_ = 10;

	rtt_ = (7 * rtt_ + keep_rtt) / 8;
	if (rtt_ < 10)
		rtt_ = 10;
	uint32_t now=rtc::TimeMillis();
	rtt_update_ts_=now;
	ping_resend_=0;

}
void PathInfo::UpdateMinRtt(uint32_t rtt){
	if(min_rtt_==0){
		min_rtt_=rtt;
	}else{
		if(rtt<min_rtt_){
			min_rtt_=rtt;
		}
	}
}
void PathInfo::SenderStop(){
	FreePendingBuf();
	FreeSentBuf();
}
void PathInfo::ReceiverStop(){

}
bool PathInfo::LossTableSeqExist(uint32_t seq){
	bool ret=false;
	for(auto it=loss_.begin();it!=loss_.end();it++){
		if((*it)==seq){
			ret=true;
			break;
		}
	}
	return ret;
}
void PathInfo::LossTableRemove(uint32_t seq){
	loss_.erase(seq);
}
void PathInfo::LossTableRemoveUntil(uint32_t seq){
	for(auto it=loss_.begin();it!=loss_.end();){
		uint32_t key=*it;
		if(key<seq){
			loss_.erase(it++);
		}else{
			break;
		}
	}
}
void PathInfo::UpdataLoss(uint32_t seq){
	uint32_t i=0;
	LossTableRemove(seq);
	for(i=max_seq_+1;i<seq;i++){
		bool exist=LossTableSeqExist(seq);
		if(!exist){
			loss_.insert(seq);
		}
	}
}
//pace queue delay at the sender side
static uint32_t smooth_num=90;
static uint32_t smooth_den=100;
void PathInfo::UpdataSendTs(uint32_t ts){
	if(s_sent_ts_==0){
		s_sent_ts_=ts;
	}else{
		s_sent_ts_=(smooth_num*s_sent_ts_+(smooth_den-smooth_num)*ts)/smooth_den;
	}
}
void PathInfo::OnReceiveSegment(sim_segment_t *seg){
	uint32_t seq=seg->packet_id;
	if(max_seq_==0&&seg->packet_id>seg->index){
		max_seq_=seg->packet_id-seg->index-1;
		base_seq_ = seg->packet_id - seg->index - 1;
	}
	UpdataSendTs(seg->send_ts);
	UpdataLoss(seq);
	max_seq_=SU_MAX(max_seq_,seq);
	if (seq == base_seq_ + 1)
		base_seq_ = seq;
	VideoRealAck(0,seq);
}
// do not wait
void PathInfo::Consume(uint32_t packet_id){
	if(base_seq_<packet_id){
		base_seq_=packet_id;
	}
    LossTableRemoveUntil(packet_id);
}
}
