#include "mpsender.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include <string>
#include <list>
#include <assert.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathSender");
static uint32_t CONN_TIMEOUT=200000;//200ms
static uint32_t CONN_RETRY=3;
MultipathSender::MultipathSender(SessionInterface *session,uint32_t uid)
:uid_(uid),pid_(1)
,session_(session)
,frame_seed_(1)
,schedule_seed_(1)
,seg_c_(0),first_ts_(-1)
,cc_type_(gcc_transport)
,min_bitrate_(MIN_BITRATE)
,start_bitrate_(START_BITRATE)
,max_bitrate_(MAX_BITRATE)
,scheduler_(NULL)
,rate_control_(NULL)
,stop_(false){
	bin_stream_init(&strm_);
	for(int i=0;i<2;i++){
		sim_segment_t *seg=new sim_segment_t();
		free_segs_.push(seg);
		seg_c_++;
	}
}
MultipathSender::~MultipathSender(){
	bin_stream_destroy(&strm_);
    NS_LOG_INFO("seg"<<std::to_string(seg_c_));
	sim_segment_t *seg=NULL;
	while(!pending_buf_.empty()){
		auto it=pending_buf_.begin();
		seg=it->second;
		pending_buf_.erase(it);
		free_segs_.push(seg);
	}
	while(!free_segs_.empty()){
		seg=free_segs_.front();
		free_segs_.pop();
		seg_c_--;
		delete seg;
	}
NS_LOG_INFO("seg"<<std::to_string(seg_c_));
}
static uint32_t PING_INTERVAL=250;
void MultipathSender::Process(){
    if(stop_){return ;}
	if(!syn_path_.empty()){
		uint32_t now=rtc::TimeMillis();
		for(auto it=syn_path_.begin();it!=syn_path_.end();){
            PathInfo *path=*it;
			if(path->con_c>CONN_RETRY){
				it=syn_path_.erase(it);
				uint8_t pid=path->pid;
				delete path;
				NS_LOG_INFO("path"<<pid<<"is unreachable");
			}else{
				uint32_t delta=now-path->con_ts;
				uint32_t backoff=path->con_c*CONN_TIMEOUT;
				if(delta>backoff){
					path->con_c++;
					path->con_ts=now;
					SendConnect(path,now);
				}
				it++;
			}
		}
	}
	uint32_t now=rtc::TimeMillis();
	PathInfo *path=NULL;
	for(auto it=usable_path_.begin();it!=usable_path_.end();it++){
		path=it->second;
		path->SenderHeartBeat(now);
		if(path->rtt_update_ts_!=0){
			if((now-path->rtt_update_ts_)>PING_INTERVAL){
				SendPingMsg(path,now);
			}
		}
	}
	SchedulePendingBuf();
}
void MultipathSender::Stop(){
	PathInfo *path=NULL;
	stop_=true;
	for(auto it=usable_path_.begin();it!=usable_path_.end();it++){
		path=it->second;
		path->SenderStop();
	}
}
void MultipathSender::Connect(su_socket *fd,su_addr *src,su_addr *dst){
	PathInfo *path=new PathInfo();
	path->fd=*fd;
	path->src=*src;
    path->dst=*dst;
	uint32_t now=rtc::TimeMillis();
	path->state=path_conning;
	path->pid=pid_;
	pid_++;
	path->con_ts=now;
	path->con_c=1;
    SendConnect(path,now);
	syn_path_.push_back(path);
}
void MultipathSender::SendPingMsg(PathInfo *path,uint32_t now){
	sim_header_t header;
	sim_ping_t body;

	INIT_SIM_HEADER(header, SIM_PING, uid_);
	header.ver=path->pid;
	body.ts = now;
	sim_encode_msg(&strm_, &header, &body);
	//not update, ping_resend_ could be exploited to detect path out
	path->rtt_update_ts_=now;
	path->ping_resend_++;
	su_udp_send(path->fd,&path->dst,strm_.data, strm_.used);
}
PathInfo* MultipathSender::GetMinRttPath(){
	PathInfo *path=NULL;
	uint32_t rtt=0;
	if(usable_path_.size()==1){
		auto it=usable_path_.begin();
		path=it->second;
	}else if(usable_path_.size()>1){
		auto it=usable_path_.begin();
		path=it->second;
		rtt=path->rtt_;
		for(;it!=usable_path_.end();it++){
			PathInfo *temp=it->second;
			if(temp->rtt_<rtt){
				path=temp;
				rtt=temp->rtt_;
			}
		}
	}
	return path;
}
void MultipathSender::SendDisconnect(PathInfo*path,uint32_t ts){
	sim_header_t header;
	sim_disconnect_t body;
	INIT_SIM_HEADER(header, SIM_DISCONNECT,uid_);
	header.ver=path->pid;
	body.cid = ts;
	sim_encode_msg(&strm_, &header, &body);
	su_udp_send(path->fd,&path->dst,strm_.data, strm_.used);
}
void MultipathSender::SendConnect(PathInfo *p,uint32_t now){
    NS_LOG_INFO("send con");
	sim_header_t header;
	sim_connect_t body;
	INIT_SIM_HEADER(header, SIM_CONNECT,uid_);
	//use protocol var to mark path id
	header.ver=p->pid;
	//use cid to compute rtt
	body.cid =now ;
	body.token_size = 0;
	body.cc_type =cc_type_;
	bin_stream_rewind(&strm_,1);
	sim_encode_msg(&strm_,&header,&body);
    NS_LOG_INFO("send con size "<<strm_.used);
	su_udp_send(p->fd,&p->dst,strm_.data, strm_.used);
}
void MultipathSender::ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream){
    if(stop_){return ;}
	uint8_t pid=header->ver;
	switch(header->mid){
	case SIM_CONNECT_ACK:{
		sim_connect_ack_t ack;
		if (sim_decode_msg(stream, header, &ack) != 0)
			return;
        printf("pid %d\n",pid);
        uint32_t now=rtc::TimeMillis();
        uint32_t rtt=now-ack.cid;
		//TODO cid can be use to updata rtt
		for(auto it=syn_path_.begin();it!=syn_path_.end();it++){
            PathInfo *path=*it;
			if(path->pid==pid){
				path->state=path_conned;
				path->rtt_=rtt;
				path->min_rtt_=rtt;
				path->rtt_update_ts_=now;
				syn_path_.erase(it);
				AddController(path);
				path->RegisterSenderInterface(this);
				usable_path_.insert(std::make_pair(pid,path));
				if(scheduler_){
					scheduler_->RegisterPath(pid);
				}
				session_->PathStateForward(NOTIFYMESSAGE::notify_con_ack,pid);
				NS_LOG_INFO("path "<<std::to_string(pid)<< "is usable rtt "<<std::to_string(rtt));
				break;
			}
		}
	}
	case SIM_SEG_ACK:{
		sim_segment_ack_t ack;
		if (sim_decode_msg(stream, header, &ack) != 0)
			return;
		InnerProcessSegAck(pid,&ack);
		break;
	}
	case SIM_FEEDBACK:{
		sim_feedback_t feedback;
		if (sim_decode_msg(stream, header, &feedback) != 0)
			return;
		InnerProcessFeedback(pid,&feedback);
		break;
	}
	}
}
void MultipathSender::ProcessPongMsg(uint8_t pid,uint32_t rtt){
    if(stop_){return ;}
	PathInfo *path=GetPathInfo(pid);
	if(path){
		path->UpdateRtt(rtt);
	}
	razor_sender_t *cc=NULL;
	cc=path->GetController()->s_cc_;
	if(cc){
		cc->update_rtt(cc,path->rtt_+path->rtt_var_);
	}
}
void MultipathSender::SchedulePendingBuf(){
	std::map<uint32_t,uint32_t> pending_info;
	{
		rtc::CritScope cs(&buf_mutex_);
		//packetid ->len
		if(!pending_buf_.empty()){
			sim_segment_t *seg=NULL;
			uint32_t id=0;
			uint32_t len=0;
			for(auto it=pending_buf_.begin();it!=pending_buf_.end();it++){
				id=it->first;
				seg=it->second;
				len=seg->data_size+SIM_SEGMENT_HEADER_SIZE;
				pending_info.insert(std::make_pair(id,len));
			}
		}
	}
	if(!pending_info.empty()){
		if(scheduler_){
			scheduler_->IncomingPackets(pending_info);
		}
	}
}
void MultipathSender::InnerProcessFeedback(uint8_t pid,sim_feedback_t* feedback){
	PathInfo *path=GetPathInfo(pid);
	razor_sender_t *cc=NULL;
	cc=path->GetController()->s_cc_;
	if(cc){
		cc->on_feedback(cc,feedback->feedback,feedback->feedback_size);
	}
}
void MultipathSender::InnerProcessSegAck(uint8_t pid,sim_segment_ack_t* ack){
	PathInfo *path=NULL;
	path=GetPathInfo(pid);
	if(path){
		path->RecvSegAck(ack);
	}
}
void MultipathSender::AddController(PathInfo *p){
	razor_sender_t *sender;
	int cc_type=cc_type_;
	int padding=1;
	CongestionController *controller=new CongestionController(this,p->pid);
	sender=razor_sender_create(cc_type,padding,
	(void*)controller,&MultipathSender::ChangeRate,
	(void*)controller,&MultipathSender::PaceSend,1000);
	controller->SetCongestion((void*)sender,zsy::ROLE::ROLE_SENDER);
	p->SetController(controller);
	sender->set_bitrates(sender,min_bitrate_,start_bitrate_,max_bitrate_);
}
#define MAX_SPLIT_NUMBER	1024
static uint16_t FrameSplit(uint16_t splits[], size_t size){
	uint16_t ret, i;
	uint16_t remain_size;

	if (size <= SIM_VIDEO_SIZE){
		ret = 1;
		splits[0] = size;
	}
	else{
		ret = size / SIM_VIDEO_SIZE;
		for (i = 0; i < ret; i++)
			splits[i] = SIM_VIDEO_SIZE;

		remain_size = size % SIM_VIDEO_SIZE;
		if (remain_size > 0){
			splits[ret] = remain_size;
			ret++;
		}
	}

	return ret;
}
//TODO
// the frame id should be set by up layer, to skip frame for rate control
//and for result comparison
void MultipathSender::SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t size){
	if(stop_){return;}
	uint32_t timestamp=0;
	int64_t now=rtc::TimeMillis();
	if(first_ts_==-1){
		first_ts_=now;
	}else{
		timestamp=now-first_ts_;
	}
	uint8_t* pos;
	uint16_t splits[MAX_SPLIT_NUMBER], total=0, i=0;
	assert((size / SIM_VIDEO_SIZE) < MAX_SPLIT_NUMBER);
	total = FrameSplit(splits, size);
	pos = (uint8_t*)data;
	std::list<sim_segment_t*> buf;
	sim_segment_t *seg=NULL;
	{
		rtc::CritScope cs(&free_mutex_);
		int new_buf=0;
		int j=0;
		if(free_segs_.size()<total){
			if(free_segs_.size()==0){
				new_buf=total;
				for(j=0;j<new_buf;j++){
					seg=new sim_segment_t();
					buf.push_back(seg);
					seg_c_++;
				}
			}else{
				new_buf=total-free_segs_.size();
				for(j=0;j<new_buf;j++){
					seg=new sim_segment_t();
					buf.push_back(seg);
					seg_c_++;
				}
				while(!free_segs_.empty()){
					seg=free_segs_.front();
					free_segs_.pop();
					buf.push_back(seg);
				}
			}
		}else{
			for(j=0;j<total;j++){
				seg=free_segs_.front();
				free_segs_.pop();
				buf.push_back(seg);
			}
		}
	}
	NS_LOG_INFO("buf_size"<<std::to_string(buf.size()));
	{
		i=0;
		rtc::CritScope cs(&buf_mutex_);
		while(!buf.empty()){
				seg=buf.front();
				buf.pop_front();
				seg->packet_id=0;
				uint32_t schedule_id=schedule_seed_;
				schedule_seed_++;
				seg->fid = frame_seed_;
				seg->timestamp = timestamp;
				seg->ftype = ftype;
				seg->payload_type = payload_type;
				seg->index = i;
				seg->total = total;
				seg->remb = 1;
				seg->send_ts = 0;
				seg->transport_seq = 0;
				seg->data_size = splits[i];
				memcpy(seg->data, pos, seg->data_size);
				pos += splits[i];
				pending_buf_.insert(std::make_pair(schedule_id,seg));
				i++;
			}
	}
    frame_seed_++;
	//SchedulePendingBuf();  pace queue is not thread safe, depress me quite a lot.
}
void MultipathSender::ChangeRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt){
	CongestionController *controller=static_cast<CongestionController*>(trigger);
	MultipathSender *mpsender=static_cast<MultipathSender*>(controller->session_);
	uint8_t pid=controller->pid_;
	if(mpsender->rate_control_){
		mpsender->rate_control_->ChangeRate(pid,bitrate,fraction_loss,rtt);
	}
}
void MultipathSender::PaceSend(void * handler, uint32_t packet_id, int retrans, size_t size, int padding){
	CongestionController *controller=static_cast<CongestionController*>(handler);
	MultipathSender *mpsender=static_cast<MultipathSender*>(controller->session_);
	uint8_t pid=controller->pid_;
	PathInfo* path=NULL;
	path=mpsender->GetPathInfo(pid);
	razor_sender_t *cc=NULL;
	cc=path->GetController()->s_cc_;
	int64_t now=rtc::TimeMillis();
	sim_header_t header;
	sim_pad_t pad;
	if(padding==1){
		pad.transport_seq =path->trans_seq_;
		path->trans_seq_++;
		pad.send_ts = (uint16_t)(now - mpsender->first_ts_);
		pad.data_size = SU_MIN(size, SIM_VIDEO_SIZE);
		INIT_SIM_HEADER(header, SIM_PAD, mpsender->uid_);
		header.ver=pid;
		sim_encode_msg(&mpsender->strm_, &header, &pad);
		if(cc){
			cc->on_send(cc, pad.transport_seq, pad.data_size + SIM_SEGMENT_HEADER_SIZE);
		}
		su_udp_send(path->fd,&path->dst,mpsender->strm_.data,mpsender->strm_.used);
	}else{
	sim_segment_t *seg=NULL;
	seg=path->get_segment(packet_id,retrans,now);
	if(seg){
		seg->transport_seq=path->trans_seq_;
		path->trans_seq_++;
		seg->send_ts = (uint16_t)(now - mpsender->first_ts_ - seg->timestamp);
		INIT_SIM_HEADER(header, SIM_SEG, mpsender->uid_);
		header.ver=pid;
		sim_encode_msg(&mpsender->strm_, &header, seg);
		if(cc){
			cc->on_send(cc,seg->transport_seq, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
		}
		su_udp_send(path->fd,&path->dst,mpsender->strm_.data,mpsender->strm_.used);
	}
	}

}
PathInfo* MultipathSender::GetPathInfo(uint8_t pid){
	PathInfo *info=NULL;
	auto it=usable_path_.find(pid);
	if(it!=usable_path_.end()){
		info=it->second;
	}
	return info;
}
//send packet id to path according to schedule;
void MultipathSender::PacketSchedule(uint32_t schedule_id,uint8_t pid){
	sim_segment_t *seg=NULL;
	{
		rtc::CritScope cs(&buf_mutex_);
		auto it=pending_buf_.find(schedule_id);
		if(it!=pending_buf_.end()){
			seg=it->second;
			pending_buf_.erase(it);
		}
	}
	if(seg){
		PathInfo *path=NULL;
		auto it=usable_path_.find(pid);
		if(it!=usable_path_.end()){
			path=it->second;
			seg->packet_id=path->packet_seed_;
			path->packet_seed_++;
			path->put(seg);
			razor_sender_t *cc=NULL;
			cc=path->GetController()->s_cc_;
			if(cc){
				cc->add_packet(cc, seg->packet_id, 0, seg->data_size + SIM_SEGMENT_HEADER_SIZE);
			}
		}else{
			NS_LOG_ERROR("fatal packet schedule error");
		}
	}
}
void  MultipathSender::SetSchedule(Schedule* s){
	scheduler_=s;
	scheduler_->SetSender(this);
}
void MultipathSender::SetRateControl(RateControl * c){
	rate_control_=c;
	rate_control_->SetSender(this);
}
void MultipathSender::Reclaim(sim_segment_t *seg){
	rtc::CritScope cs(&free_mutex_);
	free_segs_.push(seg);
}
}
