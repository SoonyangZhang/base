#include "pathsender.h"
namespace zsy{
const uint32_t kInitialBitrateBps = 500000;
PathSender::PathSender(webrtc::ProcessThread *pm,webrtc::Clock *clock)
:fd(0),pid{0}
,state(path_ini)
,con_c{0}
,con_ts(0)
,rtt_(0)
,rtt_num_(0)
,min_rtt_(0)
,rtt_var_(5)
,rtt_update_ts_(0)
,ping_resend_(0)
,trans_seq_(1)
,packet_seed_(1)
,rate_(0)
,base_seq_(0)
,s_sent_ts_(0)
,controller_(NULL)
,len_(0)
{
	bin_stream_init(&strm_);
	stop_=false;
	pm_=pm;
	clock_=clock;
}
PathSender::~PathSender(){
	bin_stream_destroy(&strm_);
	if(send_bucket_){
		delete send_bucket_;
	}

}
void PathSender::SendConnect(int cc,uint32_t now){
	sim_header_t header;
	sim_connect_t body;
	uint32_t uid=mpsender_->GetUid();
	INIT_SIM_HEADER(header, SIM_CONNECT,uid);
	//use protocol var to mark path id
	header.ver=pid;
	//use cid to compute rtt
	body.cid =now ;
	body.token_size = 0;
	body.cc_type =cc;
	bin_stream_rewind(&strm_,1);
	sim_encode_msg(&strm_,&header,&body);
	SendToNetwork(strm_.data, strm_.used);
}
void PathSender::SendDisconnect(uint32_t now){
	sim_header_t header;
	sim_disconnect_t body;
	uint32_t uid=mpsender_->GetUid();
	INIT_SIM_HEADER(header, SIM_DISCONNECT,uid);
	header.ver=pid;
	body.cid = now;
	sim_encode_msg(&strm_, &header, &body);
	SendToNetwork(strm_.data, strm_.used);
}
void PathSender::SendPingMsg(uint32_t now){
	sim_header_t header;
	sim_ping_t body;
	uint32_t uid=mpsender_->GetUid();
	INIT_SIM_HEADER(header, SIM_PING, uid);
	header.ver=pid;
	body.ts = now;
	sim_encode_msg(&strm_, &header, &body);
	//not update, ping_resend_ could be exploited to detect path out
	rtt_update_ts_=now;
	ping_resend_++;
	SendToNetwork(strm_.data, strm_.used);
}
static uint32_t max_tolerate_sent_offset=500;
static uint32_t buf_collect_time=600;
void PathSender::HeartBeat(uint32_t now){
	if(rtt_update_ts_!=0){
		if((now-rtt_update_ts_)>PING_INTERVAL){
			SendPingMsg(now);
		}
	}
	SentBufCollection(now);
}
bool PathSender::TimeToSendPacket(uint32_t ssrc,
	                              uint16_t sequence_number,
	                              int64_t capture_time_ms,
	                              bool retransmission,
	                              const webrtc::PacedPacketInfo& cluster_info){
    if(stop_){return true;}
    sim_header_t header;
	uint32_t uid=mpsender_->GetUid();
    webrtc::SendSideCongestionController *cc=NULL;
	cc=controller_->s_cc_;
	int64_t now=rtc::TimeMillis();
	sim_segment_t *seg=NULL;
	seg=get_segment(sequence_number,retransmission,now);
	if(seg){
		seg->send_ts = (uint16_t)(now - mpsender_->GetFirstTs()- seg->timestamp);
		INIT_SIM_HEADER(header, SIM_SEG, uid);
		header.ver=pid;
		sim_encode_msg(&strm_, &header, seg);
		SendToNetwork(strm_.data,strm_.used);
		rtc::SentPacket sentPacket((int64_t)sequence_number,now);
		if(cc)
		{
			cc->OnSentPacket(sentPacket);
		}
		UpdatePaceQueueDelay(seg->send_ts);
		delete seg;
	}
	return true;
}
size_t PathSender::TimeToSendPadding(size_t bytes,
	                                 const webrtc::PacedPacketInfo& cluster_info){
	printf("padding not implement");
	return bytes;
}
void PathSender::OnNetworkChanged(uint32_t bitrate_bps,
        uint8_t fraction_loss,  // 0 - 255.
        int64_t rtt_ms,
        int64_t probing_interval_ms){
	if(mpsender_){
		mpsender_->OnNetworkChanged(pid,bitrate_bps,fraction_loss,rtt_ms);
	}
}
void PathSender::ConfigureCongestion(){
	if(controller_){
		return;
	}
	send_bucket_=(new webrtc::PacedSender(clock_, this, nullptr));
	webrtc::SendSideCongestionController * cc=NULL;
	cc=new webrtc::SendSideCongestionController(clock_,this,
    		&null_log_,send_bucket_);
	controller_=new CongestionController(cc,ROLE::ROLE_SENDER);
	cc->SetBweBitrates(WEBRTC_MIN_BITRATE, kInitialBitrateBps, 5 * kInitialBitrateBps);
	send_bucket_->SetEstimatedBitrate(kInitialBitrateBps);
    send_bucket_->SetProbingEnabled(false);
    pm_->RegisterModule(send_bucket_,RTC_FROM_HERE);
    pm_->RegisterModule(cc,RTC_FROM_HERE);

}
bool PathSender::put(sim_segment_t*seg){
	if(stop_){
		return false;
	}
	seg->packet_id=packet_seed_;
	seg->transport_seq=trans_seq_;
	uint16_t id=trans_seq_;
	len_+=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	{
		rtc::CritScope cs(&buf_mutex_);
		buf_.insert(std::make_pair(id,seg));
	}
    uint32_t now=rtc::TimeMillis();
	webrtc::SendSideCongestionController * cc=NULL;
	cc=controller_->s_cc_;
	uint32_t uid=mpsender_->GetUid();
	if(cc){
		send_bucket_->InsertPacket(webrtc::PacedSender::kNormalPriority,uid,
				id,now,seg->data_size + SIM_SEGMENT_HEADER_SIZE,false);
	}
	packet_seed_++;
	trans_seq_++;
	return true;
}
sim_segment_t *PathSender::get_segment(uint16_t seq,bool retrans,uint32_t ts){
	sim_segment_t *seg=NULL;
	//send_buf_t *send_buf=NULL;
	if(stop_){
		return seg;
	}
	if(retrans==false){
		{
			rtc::CritScope cs(&buf_mutex_);
			auto it=buf_.find(seq);
			if(it!=buf_.end()){
				seg=it->second;
				len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
				buf_.erase(it);
				//send_buf=AllocateSentBuf();
				//send_buf->ts=ts;
				//send_buf->seg=seg;
			}
		}
		/*
		if(send_buf){
			rtc::CritScope cs(&sent_buf_mutex_);
			sent_buf_.push_back(send_buf);
		}*/
	}else{
		//TODO  retrans
	}
    return seg;
}
uint32_t PathSender::get_delay(){
	uint32_t delta=0;
	{
		rtc::CritScope cs(&buf_mutex_);
		if(buf_.empty()){
		}else{
			uint32_t now=rtc::TimeMillis();
			auto it=buf_.begin();
			delta=now-mpsender_->GetFirstTs()-it->second->timestamp;
		}
	}
	return delta;
}
uint32_t PathSender::get_len(){
	return len_;
}
void PathSender::UpdateMinRtt(uint32_t rtt){
	if(min_rtt_==0){
		min_rtt_=rtt;
	}else{
		if(rtt<min_rtt_){
			min_rtt_=rtt;
		}
	}
}
void PathSender::UpdateRtt(uint32_t time,uint32_t now){
	uint32_t keep_rtt=5;
	uint32_t averageRtt=0;
	if(time>keep_rtt){
		keep_rtt=time;
	}
	UpdateMinRtt(keep_rtt);
	rtt_var_ = (rtt_var_ * 3 + SU_ABS(rtt_, keep_rtt)) / 4;
	if (rtt_var_ < 10)
		rtt_var_ = 10;

	rtt_ =keep_rtt;
	if (rtt_ < 10)
		rtt_ = 10;
	rtt_update_ts_=now;
	ping_resend_=0;
    webrtc::SendSideCongestionController *cc=NULL;
    if(controller_){
    	cc=controller_->s_cc_;
    }
	if(rtt_num_==0)
	{
		if(cc){
			cc->OnRttUpdate(rtt_,rtt_);
		}
		sum_rtt_=rtt_;
		max_rtt_=rtt_;
		rtt_num_++;
		return;
	}
	rtt_num_+=1;
	sum_rtt_+=rtt_;
	if(rtt_>max_rtt_)
	{
		max_rtt_=rtt_;
	}
	averageRtt=sum_rtt_/rtt_num_;
	if(cc){
		cc->OnRttUpdate(averageRtt,max_rtt_);
	}
}
send_buf_t *  PathSender::GetSentPacketInfo(uint32_t seq){
	send_buf_t *sent_buf=NULL;
	send_buf_t *temp=NULL;
	{
		rtc::CritScope cs(&sent_buf_mutex_);
		for(auto it=sent_buf_.begin();it!=sent_buf_.end();it++){
			temp=(*it);
			if(temp->seg->packet_id>seq){
				break;
			}
			if(temp->seg->packet_id==seq){
				sent_buf=temp;
				break;
			}
		}
	}
	return sent_buf;
}
void  PathSender::RecvSegAck(sim_segment_ack_t* ack){
	if (ack->acked_packet_id >packet_seed_ || ack->base_packet_id > packet_seed_)
		return;
	uint32_t now=rtc::TimeMillis();
	send_buf_t *temp=NULL;
	temp=GetSentPacketInfo(ack->acked_packet_id);
	if(temp){
		uint32_t rtt=now-temp->ts;
		UpdateRtt(rtt,now);
	}
	SenderUpdateBase(ack->base_packet_id);
	RemoveAckedPacket(ack->acked_packet_id);
	//uint32_t minrtt=min_rtt_;
	//uint32_t i=0;
	/*  TODO
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
	}*/
}
void  PathSender::IncomingFeedBack(sim_feedback_t* feedback){
    webrtc::SendSideCongestionController *cc=NULL;
	cc=controller_->s_cc_;
	std::unique_ptr<webrtc::rtcp::TransportFeedback> fb=
	webrtc::rtcp::TransportFeedback::ParseFrom((uint8_t*)feedback->feedback, feedback->feedback_size);
	if(cc){
		cc->OnTransportFeedback(*fb.get());
	}
}
void  PathSender::SenderUpdateBase(uint32_t base_id){
	RemoveSentBufUntil(base_id);
}
void  PathSender::SendToNetwork(uint8_t*data,uint32_t len){
	su_udp_send(fd,&dst,data, len);
}
void PathSender::Stop(){
	if(stop_){
		return;
	}
	stop_=true;
    webrtc::SendSideCongestionController *cc=NULL;
	cc=controller_->s_cc_;
	pm_->DeRegisterModule(cc);
	FreePendingBuf();
	FreeSentBuf();
}
//pace queue delay at the sender side
static uint32_t smooth_num=90;
static uint32_t smooth_den=100;
static uint32_t max_sent_ts_offset=500;
void PathSender::UpdatePaceQueueDelay(uint32_t ts){
	if(ts>max_sent_ts_offset){return;}
	if(s_sent_ts_==0){
		s_sent_ts_=ts;
	}else{
		s_sent_ts_=(smooth_num*s_sent_ts_+(smooth_den-smooth_num)*ts)/smooth_den;
	}
}
send_buf_t *PathSender::AllocateSentBuf(){
	send_buf_t *buf=NULL;
	if(!buf){
		buf=new send_buf_t();
	}
	return buf;
}
void PathSender::FreePendingBuf(){
	sim_segment_t *seg=NULL;
	rtc::CritScope cs(&buf_mutex_);
	for(auto it=buf_.begin();it!=buf_.end();){
		seg=it->second;
		buf_.erase(it++);
		len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	    delete seg;
	}
}
void PathSender::FreeSentBuf(){
	send_buf_t *buf_ptr=NULL;
	sim_segment_t *seg=NULL;
	rtc::CritScope cs(&sent_buf_mutex_);
	while(!sent_buf_.empty()){
		buf_ptr=sent_buf_.front();
		sent_buf_.pop_front();
		seg=buf_ptr->seg;
		buf_ptr->seg=NULL;
		delete seg;
		delete buf_ptr;
	}
}
void  PathSender::RemoveAckedPacket(uint32_t seq){
	send_buf_t *buf_ptr=NULL;
	sim_segment_t *seg=NULL;
	rtc::CritScope cs(&sent_buf_mutex_);
	for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
		send_buf_t *temp=(*it);
		if(temp->seg->packet_id>seq){
			break;
		}
		if(temp->seg->packet_id==seq){
			buf_ptr=temp;
			sent_buf_.erase(it++);
            break;
		}else{
			it++;
		}
	}
	if(buf_ptr){
		seg=buf_ptr->seg;
		buf_ptr->seg=NULL;
		delete seg;
		delete buf_ptr;
	}
}
void PathSender::RemoveSentBufUntil(uint32_t seq){
	if(base_seq_<=seq){
		base_seq_=seq;
	}
	rtc::CritScope cs(&sent_buf_mutex_);
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
			sim_segment_t *seg=buf_ptr->seg;
			delete seg;
			delete buf_ptr;
		}
	}
}
void PathSender::SentBufCollection(uint32_t now){
	if(last_sentbuf_collect_ts_==0){
		last_sentbuf_collect_ts_=now;
				return;
	}
	uint32_t delta=now-last_sentbuf_collect_ts_;
	if(delta>buf_collect_time){
		last_sentbuf_collect_ts_=now;
		send_buf_t *ptr=NULL;
		sim_segment_t *seg=NULL;
		rtc::CritScope cs(&sent_buf_mutex_);
		for(auto it=sent_buf_.begin();it!=sent_buf_.end();){
			ptr=(*it);
			seg=ptr->seg;
			uint32_t sent_offset=now - mpsender_->GetFirstTs()- seg->timestamp;
			if(sent_offset<=max_tolerate_sent_offset){
				break;
			}
			else{
				delete seg;
				delete ptr;
				sent_buf_.erase(it++);
			}
		}
	}
}
}
