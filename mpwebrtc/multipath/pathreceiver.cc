#include "pathreceiver.h"
#include "rtc_base/location.h"
#include "log.h"
namespace zsy{
NS_LOG_COMPONENT_DEFINE("PathReceiver");
PathReceiver::PathReceiver(webrtc::ProcessThread *pm,webrtc::Clock *clock)
:fd(0),pid{0}
,state(path_ini)
,rtt_(0)
,rtt_num_(0)
,rtt_var_(5)
,rtt_update_ts_(0)
,ping_resend_(0)
,base_seq_(0)
,max_seq_(0)
,ack_ts_(0)
,controller_(NULL)
{
	bin_stream_init(&strm_);
    first_packet_=true;
	stop_=false;
	pm_=pm;
	clock_=clock;
    rbe_=NULL;
}
PathReceiver::~PathReceiver(){
	bin_stream_destroy(&strm_);
}
void PathReceiver::HeartBeat(uint32_t now){
	if((now-rtt_update_ts_)>PING_INTERVAL){
		SendPingMsg(now);
	}
}
void PathReceiver::SendConnectAck(uint32_t cid){
    //char ip_port[128]={0};
    //su_addr_to_string(&path->dst,ip_port,128);
    //NS_LOG_INFO("send ack to "<<ip_port);
	sim_header_t h;
	sim_connect_ack_t ack;
	uint32_t uid=mpreceiver_->GetUid();
	INIT_SIM_HEADER(h, SIM_CONNECT_ACK,uid);
	h.ver=pid;
	ack.cid = cid;
	ack.result = 0;
	sim_encode_msg(&strm_, &h, &ack);
	SendToNetwork(strm_.data,strm_.used);
}
void PathReceiver::SendDisconnect(uint32_t now){
	sim_header_t header;
	sim_disconnect_t body;
	uint32_t uid=mpreceiver_->GetUid();
	INIT_SIM_HEADER(header, SIM_DISCONNECT,uid);
	header.ver=pid;
	body.cid = now;
	sim_encode_msg(&strm_, &header, &body);
	SendToNetwork(strm_.data, strm_.used);
}
void PathReceiver::SendPingMsg(uint32_t now){
	sim_header_t header;
	sim_ping_t body;
	uint32_t uid=mpreceiver_->GetUid();
	INIT_SIM_HEADER(header, SIM_PING, uid);
	header.ver=pid;
	body.ts = now;
	sim_encode_msg(&strm_, &header, &body);
	//not update, ping_resend_ could be exploited to detect path out
	rtt_update_ts_=now;
	ping_resend_++;
	SendToNetwork(strm_.data, strm_.used);
}
bool PathReceiver::TimeToSendPacket(uint32_t ssrc,
	                        uint16_t sequence_number,
	                        int64_t capture_timestamp,
	                        bool retransmission,
	                        const webrtc::PacedPacketInfo& packet_info){
	return true;
}
size_t PathReceiver::TimeToSendPadding(size_t bytes,
                           const webrtc::PacedPacketInfo& packet_info){
	return bytes;
}
uint16_t PathReceiver::AllocateSequenceNumber(){
	return 0;
}
void PathReceiver::OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
                               uint32_t bitrate_bps){
	return ;
}
bool PathReceiver::SendTransportFeedback(webrtc::rtcp::TransportFeedback* packet){
	uint32_t uid=mpreceiver_->GetUid();
	packet->SetSenderSsrc(uid);
	rtc::Buffer serialized= packet->Build();
	sim_header_t header;
	sim_feedback_t feedback;
    uint32_t payload_size=serialized.size();
	if (payload_size > SIM_FEEDBACK_SIZE){
		NS_LOG_ERROR("feedback size > SIM_FEEDBACK_SIZE");
		return false;
	}
	INIT_SIM_HEADER(header, SIM_FEEDBACK, uid);
    header.ver=pid;
	feedback.base_packet_id =base_seq_;
	feedback.feedback_size = payload_size;
	memcpy(feedback.feedback, (uint8_t*)serialized.data(), payload_size);
	sim_encode_msg(&strm_, &header, &feedback);
	SendToNetwork(strm_.data,strm_.used);
    return true;
}
void PathReceiver::UpdateRtt(uint32_t time,uint32_t now){
	uint32_t keep_rtt=5;
	uint32_t averageRtt=0;
	if(time>keep_rtt){
		keep_rtt=time;
	}
	rtt_var_ = (rtt_var_ * 3 + SU_ABS(rtt_, keep_rtt)) / 4;
	if (rtt_var_ < 10)
		rtt_var_ = 10;

	rtt_ =keep_rtt;
	if (rtt_ < 10)
		rtt_ = 10;
	rtt_update_ts_=now;
	ping_resend_=0;
	webrtc::ReceiveSideCongestionController *cc=NULL;
	if(controller_){
		cc=controller_->r_cc_;
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
void PathReceiver::OnReceiveSegment(sim_header_t *header,sim_segment_t *seg){
	if(stop_){
		return;
	}
	if(first_packet_){
		ConfigureCongestion();
		first_packet_=false;
	}
	{
		uint32_t now=rtc::TimeMillis();
		uint32_t overhead=seg->data_size + SIM_SEGMENT_HEADER_SIZE;
		webrtc::RTPHeader fakeHeader;
		fakeHeader.ssrc=header->uid;
		fakeHeader.extension.hasTransportSequenceNumber=true;
		fakeHeader.extension.transportSequenceNumber=seg->transport_seq;
		webrtc::ReceiveSideCongestionController *cc=NULL;
		cc=controller_->r_cc_;
		cc->OnReceivedPacket(now,overhead,fakeHeader);
	}
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
    printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
    		pid,seg->fid,seq,base_seq_,max_seq_,seg->ftype,min_seq,loss_size,delay);
}
#define PATH_ACK_REAL_TIME	20
#define PATH_ACK_HB_TIME		200
void PathReceiver::VideoRealAck(int hb,uint32_t seq){
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
void PathReceiver::SendSegmentAck(sim_segment_ack_t *ack){
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
void PathReceiver::Consume(uint32_t packet_id){
    LossTableRemoveUntil(packet_id);
    RecvTableRemoveUntil(packet_id);
}
void PathReceiver::SendToNetwork(uint8_t*data,uint32_t len){
	su_udp_send(fd,&dst,data, len);
}
void PathReceiver::Stop(){
	if(stop_==true){
		return;
	}
	stop_=true;
    if(rbe_)
    {
        pm_->DeRegisterModule(rbe_);
    }
	webrtc::ReceiveSideCongestionController *cc=NULL;
	if(controller_){
		cc=controller_->r_cc_;
		pm_->DeRegisterModule(cc);
	}
}
void PathReceiver::ConfigureCongestion(){
	webrtc::ReceiveSideCongestionController *cc=NULL;
	cc=new webrtc::ReceiveSideCongestionController(
						clock_,this);
	rbe_=cc->GetRemoteBitrateEstimator(true);
    pm_->RegisterModule(rbe_,RTC_FROM_HERE);
	pm_->RegisterModule(cc,RTC_FROM_HERE);
	controller_=new CongestionController(cc,ROLE::ROLE_RECEIVER);
}
bool PathReceiver::LossTableSeqExist(uint32_t seq){
	bool ret=false;
	for(auto it=loss_.begin();it!=loss_.end();it++){
		if((*it)==seq){
			ret=true;
			break;
		}
	}
	return ret;
}
void PathReceiver::LossTableRemove(uint32_t seq){
	loss_.erase(seq);
}
void PathReceiver::LossTableRemoveUntil(uint32_t seq){
	for(auto it=loss_.begin();it!=loss_.end();){
		uint32_t key=*it;
		if(key<=seq){
			loss_.erase(it++);
		}else{
			break;
		}
	}
}
void PathReceiver::UpdataLoss(uint32_t seq){
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
void PathReceiver::UpdataRecvTable(uint32_t seq){
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
void PathReceiver::RecvTableRemoveUntil(uint32_t seq){
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
bool PathReceiver::CheckRecvTableExist(uint32_t seq){
    bool ret=false;
    auto it=recv_table_.find(seq);
    if(it!=recv_table_.end()){
    ret=true;
    }
    return ret;
}
uint32_t PathReceiver::GetMinRecvSeq(){
    uint32_t seq=0;
    if(!recv_table_.empty()){
    auto it=recv_table_.begin();
    seq=(*it);
    }
    return seq;
}
static uint32_t protection_time=50;
//50ms caution, relate to pace buffer threshold
uint32_t PathReceiver::GetWaittingDuration(){
	uint32_t ret=0;
	ret=rtt_+rtt_var_+protection_time;
	return ret;
}
uint32_t PathReceiver::GetLossTableSize(){
    uint32_t len=loss_.size();
    return len;
}
}




