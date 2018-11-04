#include "pathsender.h"
#include "ns3/processmodule.h"
#include "ns3/log.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE("PathSender");
const uint32_t kInitialBitrateBps = 500000;
#define WEBRTC_MIN_BITRATE		80000
static const int kTargetBitrateBps = 800000;
constexpr unsigned kFirstClusterBps = 900000;
constexpr unsigned kSecondClusterBps = 1800000;

// The error stems from truncating the time interval of probe packets to integer
// values. This results in probing slightly higher than the target bitrate.
// For 1.8 Mbps, this comes to be about 120 kbps with 1200 probe packets.
constexpr int kBitrateProbingError = 150000;

const float kPaceMultiplier = 2.5f;

static uint32_t CONN_TIMEOUT=500;//200ms
static uint32_t CONN_RETRY=3;
PathSender::PathSender()
:pid{0}
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
	first_packet_=false;
	pm_=new ProcessModule();
	clock_=NULL;
    observer_=NULL;
}
PathSender::~PathSender(){
	bin_stream_destroy(&strm_);
	if(send_bucket_){
		delete send_bucket_;
	}
	delete pm_;
    if(observer_){
        delete observer_;
    }
}
void PathSender::SetClock(webrtc::Clock *clock){
	clock_=clock;
}
void PathSender::SendConnect(){
	if(state==path_ini||state==path_conning){
		sim_header_t header;
		sim_connect_t body;
		uint32_t now=Simulator::Now().GetMilliSeconds();
		uint32_t uid=mpsender_->GetUid();
		INIT_SIM_HEADER(header, SIM_CONNECT,uid);
		//use protocol var to mark path id
		header.ver=pid;
		//use cid to compute rtt
		body.cid =now ;
		body.token_size = 0;
		body.cc_type =0;
		bin_stream_rewind(&strm_,1);
		sim_encode_msg(&strm_,&header,&body);
		SendToNetwork(strm_.data, strm_.used);
		state=path_conning;
		con_c++;
		uint32_t backoff=con_c*CONN_TIMEOUT;
		Time next=MilliSeconds(backoff);//100ms
		rxTimer_=Simulator::Schedule(next,&PathSender::SendConnect,this);
        NS_LOG_INFO("send con");
	}
	if(state==path_conned){
		rxTimer_.Cancel();
	}
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
static uint32_t buf_collect_time=500;
void PathSender::HeartBeat(){
	if(gcTimer_.IsExpired()){
		uint32_t now=Simulator::Now().GetMilliSeconds();
		SentBufCollection(now);
		Time next=MilliSeconds(buf_collect_time);//500ms
		gcTimer_=Simulator::Schedule(next,&PathSender::HeartBeat,this);
	}
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
        if(cc){
        cc->AddPacket(uid,sequence_number,seg->data_size+SIM_SEGMENT_HEADER_SIZE,webrtc::PacedPacketInfo());
        }
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
void PathSender::ConfigureCongestion(){
	if(controller_){
		return;
	}
    observer_=new ProxyObserver(mpsender_,pid);
	send_bucket_=new webrtc::PacedSender(&m_clock, this, nullptr);
	webrtc::SendSideCongestionController * cc=NULL;
	cc=new webrtc::SendSideCongestionController(&m_clock,observer_,
    		&null_log_,send_bucket_);
	controller_=new CongestionController(cc,ROLE::ROLE_SENDER);
	cc->SetBweBitrates(WEBRTC_MIN_BITRATE, kInitialBitrateBps, 5 * kInitialBitrateBps);
	send_bucket_->SetEstimatedBitrate(kInitialBitrateBps);
    send_bucket_->SetProbingEnabled(false);
    pm_->RegisterModule(send_bucket_,RTC_FROM_HERE);
    pm_->RegisterModule(cc,RTC_FROM_HERE);
    pm_->Start();
}
bool PathSender::put(sim_segment_t*seg){
	if(stop_){
		return false;
	}
	if(!first_packet_){
		first_packet_=true;
		Time next=MilliSeconds(buf_collect_time);//500ms
		gcTimer_=Simulator::Schedule(next,&PathSender::HeartBeat,this);
		HeartBeat();
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
				id,now,seg->data_size/* + SIM_SEGMENT_HEADER_SIZE*/,false);
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

	rtt_ = (7 * rtt_ + keep_rtt) / 8;
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
			cc->OnRttUpdate(keep_rtt,keep_rtt);
		}
		sum_rtt_=keep_rtt;
		max_rtt_=keep_rtt;
		rtt_num_++;
		return;
	}
	rtt_num_+=1;
	sum_rtt_+=keep_rtt;
	if(keep_rtt>max_rtt_)
	{
		max_rtt_=keep_rtt;
	}
	averageRtt=sum_rtt_/rtt_num_;
	if(cc){
        //NS_LOG_INFO("average "<<averageRtt<<"MAX "<<max_rtt_);
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
        //printf("fb\n");
		cc->OnTransportFeedback(*fb.get());
	}
}
void  PathSender::SenderUpdateBase(uint32_t base_id){
	RemoveSentBufUntil(base_id);
}
void  PathSender::SendToNetwork(uint8_t*data,uint32_t len){
	if(socket_==NULL){
		NS_LOG_ERROR("socket is null");
		return;
	}
    MpvideoHeader header1;
    header1.SetMid(PayloadType::razor);
	Ptr<Packet> p=Create<Packet>((uint8_t*)data,len);
    p->AddHeader(header1);
	socket_->SendTo(p,0,InetSocketAddress{peer_ip_,peer_port_});
}
void PathSender::Stop(){
	if(stop_){
		return;
	}
	stop_=true;
	gcTimer_.Cancel();
	pingTimer_.Cancel();
    webrtc::SendSideCongestionController *cc=NULL;
	cc=controller_->s_cc_;
	pm_->DeRegisterModule(send_bucket_);
	pm_->DeRegisterModule(cc);
	pm_->Stop();
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
void PathSender::Bind(uint16_t port){
    if (socket_ == NULL) {
        socket_ = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
        auto local = InetSocketAddress{Ipv4Address::GetAny (), port};
        auto res = socket_->Bind (local);
        NS_ASSERT (res == 0);
    }
    bind_port_=port;
    socket_->SetRecvCallback (MakeCallback(&PathSender::RecvPacket,this));
}
InetSocketAddress PathSender::GetLocalAddress(){
    Ptr<Node> node=GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    Ipv4Address local_ip = ipv4->GetAddress (1, 0).GetLocal ();
	return InetSocketAddress{local_ip,bind_port_};
}
void PathSender::CheckPing(){
	uint32_t now=Simulator::Now().GetMilliSeconds();
	if(pingTimer_.IsExpired()){
		if((now-rtt_update_ts_)>PING_INTERVAL){
			SendPingMsg(now);
		}
		Time next=MilliSeconds(PING_INTERVAL);
		pingTimer_=Simulator::Schedule(next,&PathSender::CheckPing,this);
	}
}
void PathSender::StartApplication(){
//nothing
}
void PathSender::StopApplication(){
    if(mpsender_)
    {
        mpsender_->StopCallback();
    }
}
void PathSender::RecvPacket(Ptr<Socket> socket){
	Address remoteAddr;
    auto packet = socket->RecvFrom (remoteAddr);
    MpvideoHeader header1;
    packet->RemoveHeader(header1);
    if(header1.GetPayloadType()==PayloadType::razor)
    {
    bin_stream_rewind(&strm_, 1);
    uint32_t len=packet->GetSize ();
	packet->CopyData ((uint8_t*)strm_.data,len);
    strm_.used=len;
	ProcessingMsg(&strm_);
    }
    if(header1.GetPayloadType()==PayloadType::webrtc_fb){
        uint8_t buffer[packet->GetSize()];
		packet->CopyData ((uint8_t*)&buffer, packet->GetSize ());
		std::unique_ptr<webrtc::rtcp::TransportFeedback> fb=
		webrtc::rtcp::TransportFeedback::ParseFrom((uint8_t*)&buffer, packet->GetSize ());
        webrtc::SendSideCongestionController *cc=NULL;
        cc=controller_->s_cc_;
	if(cc){
        //printf("feedback\n");
		cc->OnTransportFeedback(*fb.get());
	}        
    }

}
void PathSender::ProcessingMsg(bin_stream_t *stream){
	if(stop_){
		return;
	}
	sim_header_t header;
	if (sim_decode_header(stream, &header) != 0)
		return;
	uint8_t temp_pid=header.ver;
	if(temp_pid!=pid){
		NS_LOG_ERROR("pid error");
		return;
	}
	if (header.mid < MIN_MSG_ID || header.mid > MAX_MSG_ID)
		return;
	switch(header.mid){
	case SIM_CONNECT_ACK:{
		sim_connect_ack_t ack;
		if (sim_decode_msg(stream, &header, &ack) != 0)
			return;
        uint32_t now=Simulator::Now().GetMilliSeconds();
        rtt_=now-ack.cid;
        rtt_update_ts_=now;
		if(mpsender_&&(state!=path_conned)){
             NS_LOG_INFO("CON ACK");
			mpsender_->NotifyPathUsable(pid);
			ConfigureCongestion();
            CheckPing();
		}
        state=path_conned;
		//TODO start ping timer;
		break;
	}
	case SIM_SEG_ACK:{
		sim_segment_ack_t ack;
		if (sim_decode_msg(stream, &header, &ack) != 0)
			return;
		RecvSegAck(&ack);
		break;
	}
	case SIM_FEEDBACK:{
		sim_feedback_t feedback;
		if (sim_decode_msg(stream, &header, &feedback) != 0)
			return;
		IncomingFeedBack(&feedback);
		break;
	}
	case SIM_PING:{
		sim_header_t h;
		sim_ping_t ping;
		sim_decode_msg(stream, &header, &ping);
		uint32_t uid=mpsender_->GetUid();
		INIT_SIM_HEADER(h, SIM_PONG, uid);
        h.ver=pid;
		sim_encode_msg(stream, &h, &ping);
		SendToNetwork(stream->data,stream->used);
        break;
	}
	case SIM_PONG:{
		sim_pong_t pong;
		sim_decode_msg(stream, &header, &pong);
		uint32_t now=Simulator::Now().GetMilliSeconds();
		uint32_t rtt=now-pong.ts;
		if(rtt>0){
			UpdateRtt(rtt,now);
		}
        break;
	}
    
	default:{
		break;
	}
	}
}
}
