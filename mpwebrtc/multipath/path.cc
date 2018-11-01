#include "path.h"
#include "sim_proto.h"
#include "rtc_base/timeutils.h"
#include "log.h"
#include <string>
#include<stdio.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("PathInfo");
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
,sender_last_heart_beat_(0)
,last_sentbuf_collect_ts_(0){
	bin_stream_init(&strm_);    
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
    bin_stream_destroy(&strm_);
}
void PathInfo::OnNetworkChanged(uint32_t bitrate_bps,
                      uint8_t fraction_loss,  // 0 - 255.
                      int64_t rtt_ms,
                      int64_t probing_interval_ms){
	if(mpsender_){
		mpsender_->OnNetworkChanged(pid,bitrate_bps,fraction_loss,rtt_ms);
	}
}
static uint32_t max_tolerate_sent_offset=500;
static uint32_t buf_collect_time=600;
void PathInfo::SenderHeartBeat(uint32_t ts){
	if(sender_last_heart_beat_+5<ts){
		razor_sender_t *cc=NULL;
		cc=controller_->s_cc_;
		if(cc){
			cc->heartbeat(cc);
		}
		sender_last_heart_beat_=ts;
	}
	SentBufCollection(ts);
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
	seg->packet_id=packet_seed_;
	packet_seed_++;
	uint32_t id=seg->packet_id;
	auto it=buf_.find(id);
	if(it!=buf_.end()){
		return false;
	}
	//printf("%d send buf %d\n",pid,buf_.size());
	len_+=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	buf_.insert(std::make_pair(id,seg));
	razor_sender_t *cc=NULL;
	cc=GetController()->s_cc_;
	if(cc){
		cc->add_packet(cc, seg->packet_id, 0, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
	}
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
		for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
			send_buf_t *ptr=(*it);
			if(ptr->seg->packet_id>id){
				// the sent_buf_ is in order;
				break;
			}
			if(ptr->seg->packet_id==id){
				seg=ptr->seg;
				break;
			}else{
				it++;
			}
		}
	}
    //NS_LOG_INFO("path pending sent"<<std::to_string(buf_.size())
    //            <<" "<<std::to_string(sent_buf_.size()));
    uint32_t sent_size=sent_buf_.size();
    if(sent_size>500)
    {
    printf(" p %d sent %u\n",pid,sent_size);
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
void PathInfo::ConfigureSendeeSideCongestion(){

}
void PathInfo::ConfigureReceiverSideCongestion(){

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
		auto it=loss_.begin();
		if(loss_.size()>NACK_NUM){
			uint32_t i=0;
			uint32_t total=loss_.size()-NACK_NUM;
			for(i=0;i<total;i++){
				it++;
			}
		}
		for(;it!=loss_.end();it++){
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
        //NS_LOG_INFO("ack id " <<std::to_string(ack.base_packet_id));
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
		temp=NULL;//test not retrasmission//GetSentPacketInfo(seq);
		if(temp){
			seg=temp->seg;
			uint32_t sent_offset=now - mpsender_->GetFirstTs()- seg->timestamp;
			if(sent_offset<max_tolerate_sent_offset){
				if((now-temp->ts)>minrtt){
					temp->ts=now;
					if(cc){
						cc->add_packet(cc,seg->packet_id, 1, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
					}
				}
			}else{
				RemoveSentBufUntil(seg->packet_id);
			}

		}
	}

}
void PathInfo::IncomingFeedBack(sim_feedback_t* feedback){
	razor_sender_t *cc=NULL;
	cc=GetController()->s_cc_;
	if(cc){
		cc->on_feedback(cc,feedback->feedback,feedback->feedback_size);
	}
}
void PathInfo::SenderUpdateBase(uint32_t base_id){
    RemoveSentBufUntil(base_id);  
}
void PathInfo::RemoveAckedPacket(uint32_t seq){
	send_buf_t *buf_ptr=NULL;
	for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
		if((*it)->seg->packet_id==seq){
			buf_ptr=(*it);
			sent_buf_.erase(it++);
            break;
		}else{
			it++;
		}
	}
	if(buf_ptr){
		mpsender_->Reclaim(buf_ptr->seg);
		buf_ptr->seg=NULL;
		memset(buf_ptr,0,sizeof(send_buf_t));
		free_buf_.push(buf_ptr);
	}
}
void PathInfo::RemoveSentBufUntil(uint32_t seq){
	if(base_seq_<=seq){
		base_seq_=seq;
	}
	send_buf_t *buf_ptr=NULL;
	for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
		uint32_t id=(*it)->seg->packet_id;
		buf_ptr=NULL;
		if(id<=seq){
			buf_ptr=(*it);
			sent_buf_.erase(it++);
		}else{
			break;
		}
		if(buf_ptr){
			mpsender_->Reclaim(buf_ptr->seg);
			buf_ptr->seg=NULL;
			memset(buf_ptr,0,sizeof(send_buf_t));
			free_buf_.push(buf_ptr);
		}
	}
}
void PathInfo::SentBufCollection(uint32_t now){
	if(last_sentbuf_collect_ts_==0){
		last_sentbuf_collect_ts_=now;
				return;
	}
	uint32_t delta=now-last_sentbuf_collect_ts_;
	if(delta>buf_collect_time){
		last_sentbuf_collect_ts_=now;
		send_buf_t *ptr=NULL;
		sim_segment_t *seg=NULL;
		for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
			ptr=(*it);
			seg=ptr->seg;
			uint32_t sent_offset=now - mpsender_->GetFirstTs()- seg->timestamp;
			if(sent_offset<=max_tolerate_sent_offset){
				break;
			}
			else{
				sent_buf_.erase(it++);
				mpsender_->Reclaim(seg);
				ptr->seg=NULL;
				memset(ptr,0,sizeof(send_buf_t));
				free_buf_.push(ptr);
			}
		}
	}
}
void PathInfo::SendSegmentAck(sim_segment_ack_t *ack){
    uint32_t uid=0;
	if(mpreceiver_){
		uid=mpreceiver_->GetUid();
	}
	sim_header_t header;
	INIT_SIM_HEADER(header, SIM_SEG_ACK, uid);
	header.ver=pid;
	sim_encode_msg(&strm_, &header, ack);
	SendToNetwork(strm_.data,strm_.used);
}
void PathInfo::SendFeedback(const uint8_t* payload, int payload_size){
	sim_header_t header;
	sim_feedback_t feedback;
	if (payload_size > SIM_FEEDBACK_SIZE){
		NS_LOG_ERROR("feedback size > SIM_FEEDBACK_SIZE");
		return;
	}
    uint32_t uid=0;
	if(mpreceiver_){
		uid=mpreceiver_->GetUid();
	}
	INIT_SIM_HEADER(header, SIM_FEEDBACK, uid);
    header.ver=pid;
	feedback.base_packet_id =base_seq_;
	feedback.feedback_size = payload_size;
	memcpy(feedback.feedback, payload, payload_size);
	sim_encode_msg(&strm_, &header, &feedback);
	SendToNetwork(strm_.data,strm_.used);
}
void PathInfo::PaceSend(uint32_t packet_id, int retrans, size_t size, int padding){
    uint32_t uid=mpsender_->GetUid();
	razor_sender_t *cc=NULL;
	cc=GetController()->s_cc_;
	int64_t now=rtc::TimeMillis();
	sim_header_t header;
	sim_pad_t pad;
	if(padding==1){
		pad.transport_seq =trans_seq_;
		trans_seq_++;
		pad.send_ts = (uint16_t)(now - mpsender_->GetFirstTs());
		pad.data_size = SU_MIN(size, SIM_VIDEO_SIZE);
		INIT_SIM_HEADER(header, SIM_PAD, uid);
		header.ver=pid;
		sim_encode_msg(&strm_, &header, &pad);
		if(cc){
			cc->on_send(cc, pad.transport_seq, pad.data_size + SIM_SEGMENT_HEADER_SIZE);
		}
		SendToNetwork(strm_.data,strm_.used);
	}else{
	sim_segment_t *seg=NULL;
	seg=get_segment(packet_id,retrans,now);
	if(seg){
		seg->transport_seq=trans_seq_;
		trans_seq_++;
		seg->send_ts = (uint16_t)(now - mpsender_->GetFirstTs()- seg->timestamp);
		INIT_SIM_HEADER(header, SIM_SEG, uid);
		header.ver=pid;
		sim_encode_msg(&strm_, &header, seg);
		if(cc){
			cc->on_send(cc,seg->transport_seq, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
		}
		SendToNetwork(strm_.data,strm_.used);
	}
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
		if(key<=seq){
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
		bool exist=LossTableSeqExist(i);
		if(!exist){
            printf("l %d\t%d\n",pid,i);
			loss_.insert(i);
		}
	}
}
//pace queue delay at the sender side
static uint32_t smooth_num=90;
static uint32_t smooth_den=100;
static uint32_t max_sent_ts_offset=500;
void PathInfo::UpdataSendTs(uint32_t ts){
	if(ts>max_sent_ts_offset){return;}
	if(s_sent_ts_==0){
		s_sent_ts_=ts;
	}else{
		s_sent_ts_=(smooth_num*s_sent_ts_+(smooth_den-smooth_num)*ts)/smooth_den;
	}
}
uint32_t PathInfo::GetMinRecvSeq(){
    uint32_t seq=0;
    if(!recv_table_.empty()){
    auto it=recv_table_.begin();
    seq=(*it);
    }
return seq;
}
uint32_t PathInfo::GetLossTableSize(){
    uint32_t len=loss_.size();
    return len;
}
void PathInfo::UpdataRecvTable(uint32_t seq){
	if(seq==base_seq_+1){
		base_seq_=seq;
	}else{
		recv_table_.insert(seq);
	}
	while(!recv_table_.empty()){
		auto it=recv_table_.begin();
		uint32_t key=(*it);
		if(key<=base_seq_){
			recv_table_.erase(it);
		}else{
			break;
		}
	}    
	while(!recv_table_.empty()){
		auto it=recv_table_.begin();
		uint32_t key=(*it);
		if(key==base_seq_+1){
			recv_table_.erase(it);
			base_seq_=key;
		}else{
			break;
		}
	}
}
void PathInfo::RecvTableRemoveUntil(uint32_t seq){
    //printf("r %d\t%d\n",base_seq_,seq);
	while(!recv_table_.empty()){
		auto it=recv_table_.begin();
		uint32_t key=(*it);
		if(key<=seq){
			recv_table_.erase(it);
		}else{
			break;
		}
	}
	if(base_seq_<=seq){
		base_seq_=seq;
	}
	while(!recv_table_.empty()){
		auto it=recv_table_.begin();
		uint32_t key=(*it);
		if(key==base_seq_+1){
			recv_table_.erase(it);
			base_seq_=key;
		}else{
			break;
		}
    }
}
bool PathInfo::CheckRecvTableExist(uint32_t seq){
    bool ret=false;
    auto it=recv_table_.find(seq);
    if(it!=recv_table_.end()){
    ret=true;
    }
    return ret;
   }
void PathInfo::OnReceiveSegment(sim_segment_t *seg){
	uint32_t seq=seg->packet_id;
    if(CheckRecvTableExist(seq))
    {
        return;
    }
    if(seq<base_seq_){
        printf("e%d\t%d\t%d\t\n",pid,seq,base_seq_);
        return ;
    }
	if(max_seq_==0&&seg->packet_id>seg->index){
		max_seq_=seg->packet_id-seg->index-1;
		base_seq_ = seg->packet_id - seg->index - 1;
	}  
	UpdataSendTs(seg->send_ts);
	UpdataLoss(seq);
	max_seq_=SU_MAX(max_seq_,seq);
	UpdataRecvTable(seq);
	VideoRealAck(0,seq);
	if(mpreceiver_){
		mpreceiver_->DeliverToCache(pid,seg);
	} 
    uint32_t min_seq=GetMinRecvSeq();
    uint32_t loss_size=GetLossTableSize();
    uint32_t delay=seg->send_ts; 
    printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",pid,seg->fid,seq,base_seq_,max_seq_,seg->ftype,min_seq,loss_size,delay);
}
// do not wait
void PathInfo::Consume(uint32_t packet_id){
    LossTableRemoveUntil(packet_id);
    RecvTableRemoveUntil(packet_id);
}
void PathInfo::SendToNetwork(uint8_t*data,uint32_t len){
	su_udp_send(fd,&dst,data, len);
}
}
