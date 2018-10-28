#include "mpreceiver.h"
#include "sim_external.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include<memory.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");
MultipathReceiver::MultipathReceiver(SessionInterface*session,uint32_t uid)
:uid_(uid)
,min_bitrate_(MIN_BITRATE)
,max_bitrate_(MAX_BITRATE)
,session_(session)
,seg_c_(0)
,deliver_(NULL),
stop_(false){
	bin_stream_init(&strm_);
	for(int i=0;i<2;i++){
		video_packet_t *v_packet=new video_packet_t();
		free_segs_.push(v_packet);
		seg_c_++;
	}
}
MultipathReceiver::~MultipathReceiver(){
	bin_stream_destroy(&strm_);
	video_packet_t *v_packet=NULL;
	while(!free_segs_.empty()){
		v_packet=free_segs_.front();
		free_segs_.pop();
		seg_c_--;
		delete v_packet;
	}
}
bool MultipathReceiver::RegisterDataSink(NetworkDataConsumer* c){
    bool ret=false;
    if(!deliver_){
    deliver_=c;
    ret=true;
    }
    return ret;    
}
void MultipathReceiver::SendSegmentAck(uint8_t pid,sim_segment_ack_t *ack){
	sim_header_t header;
	INIT_SIM_HEADER(header, SIM_SEG_ACK, uid_);
	header.ver=pid;
	sim_encode_msg(&strm_, &header, ack);
	PathInfo *path=GetPath(pid);
	su_udp_send(path->fd,&path->dst,strm_.data,strm_.used);
}
void MultipathReceiver::SendPingMsg(PathInfo*path,uint32_t now){
	sim_header_t header;
	sim_ping_t body;
    //NS_LOG_INFO("PING");
	INIT_SIM_HEADER(header, SIM_PING, uid_);
	header.ver=path->pid;
	body.ts = now;
	sim_encode_msg(&strm_, &header, &body);
	path->rtt_update_ts_=now;
	path->ping_resend_++;
	su_udp_send(path->fd,&path->dst,strm_.data, strm_.used);
}
void MultipathReceiver::ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header
		,bin_stream_t *stream){
    if(stop_){return ;}
	switch (header->mid){
	case SIM_CONNECT:
	{
 		sim_connect_t body;
		if (sim_decode_msg(stream, header, &body) != 0)
			return;
		uint32_t cid=body.cid;
		uint8_t cc_type=body.cc_type;
		uint8_t pid=header->ver;
		PathInfo *path=GetPath(pid);
		if(!path){
			path=new PathInfo();
			path->pid=pid;
			path->fd=*fd;
			path->dst=*remote;
			path->state=path_conned;
			path->RegisterReceiverInterface(this);
			session_->Fd2Addr(*fd,&path->src);
            paths_.insert(std::make_pair(pid,path));
			ConfigureController(path,cc_type);
			session_->PathStateForward(NOTIFYMESSAGE::notify_con,pid);
		}
		SendConnectAck(path,cid);
        break;
	}
	case SIM_SEG:{
		sim_segment_t body;
		uint8_t pid=header->ver;
		if (sim_decode_msg(stream, header, &body) != 0){
			return;
		}
		ProcessSegMsg(pid,&body);
		break;
	}
	case SIM_PAD:{
		uint8_t pid=header->ver;
		PathInfo *path=GetPath(pid);
		sim_pad_t pad;
		if (sim_decode_msg(stream, header, &pad) != 0)
			return;
		if(path){
			razor_receiver_t *cc=NULL;
			cc=path->GetController()->r_cc_;
			if(cc){
				cc->on_received(cc,pad.transport_seq, pad.send_ts, pad.data_size+SIM_SEGMENT_HEADER_SIZE,1);
			}
		}
		break;
	}
	}
}
PathInfo *MultipathReceiver::GetPath(uint8_t pid){
	PathInfo *path=NULL;
	auto it=paths_.find(pid);
	if(it!=paths_.end()){
		path=it->second;
	}
	return path;
}
razor_receiver_t* MultipathReceiver::GetRazorCC(uint32_t pid){
	razor_receiver_t *cc=NULL;
	PathInfo *path=GetPath(pid);
	if(path){
		cc=path->GetController()->r_cc_;
	}
	return cc;
}
void MultipathReceiver::ProcessSegMsg(uint8_t pid,sim_segment_t*data){
	if(stop_){return;}
	PathInfo *path=GetPath(pid);
	if(path==NULL){
		return;
	}
	razor_receiver_t *cc=NULL;
	cc=path->GetController()->r_cc_;
	if(cc){
		cc->on_received(cc,data->transport_seq,data->timestamp + data->send_ts,
				data->data_size + SIM_SEGMENT_HEADER_SIZE, data->remb);
	}
	path->OnReceiveSegment(data);
	DeliverToCache(pid,data);

}
bool MultipathReceiver::DeliverFrame(video_frame_t *f){
	bool ret=false;
	uint32_t fid=f->fid;
	if(deliver_){
		uint32_t size=0;
		uint32_t buf_size=0;
		uint32_t i=0;
		video_packet_t *packet=NULL;
		for(i=0;i<f->total;i++){
		packet=f->packets[i];
			if(packet){
				buf_size+=packet->seg.data_size;
			}
		}
		uint8_t *buf=new uint8_t[buf_size];
		uint32_t len=0;
		uint8_t offset=0;
		uint8_t *data;
		for(i=0;i<f->total;i++){
			packet=f->packets[i];
			if(packet){
				len=packet->seg.data_size;
				data=(uint8_t*)(packet->seg.data);
				memcpy(buf+offset,data,len);
				offset+=len;
			}
		}
		deliver_->ForwardUp(fid,buf,offset,f->recv,f->total);
		delete [] buf;
		ret=true;
	}
	return ret;
}
void MultipathReceiver::FrameConsumed(video_frame_t *f){
	video_packet_t *packet=NULL;
	PathInfo *path=NULL;
	uint8_t pid=0;
	uint32_t packet_id=0;
	uint32_t i=0;
	for(i=0;i<f->total;i++){
		packet=f->packets[i];
		if(packet){
			pid=packet->pid;
			packet_id=packet->seg.packet_id;
			path=GetPath(pid);
			if(path){
				path->Consume(packet_id);
			}
		}
	}
}
void MultipathReceiver::BuffCollection(video_frame_t*f){
	uint32_t i=0;
	video_packet_t *packet=NULL;
	for(i=0;i<f->total;i++){
		packet=f->packets[i];
		if(packet){
			free_segs_.push(packet);
			f->packets[i]=NULL;
		}
	}
}
void MultipathReceiver::CheckDeliverFrame(){
	video_frame_t *frame=NULL;
	video_frame_t *waitting_for_delete=NULL;
	uint32_t now=rtc::TimeMillis();
	for(auto it=frame_cache_.begin();it!=frame_cache_.end();){
		frame=it->second;
		waitting_for_delete=NULL;
		if(frame->recv==frame->total){
			DeliverFrame(frame);
			FrameConsumed(frame);
			waitting_for_delete=frame;
		}else if(frame->recv<frame->total){
			if(frame->waitting_ts!=-1){
				if(now>frame->waitting_ts){
					DeliverFrame(frame);
					FrameConsumed(frame);
					waitting_for_delete=frame;
				}
			}else{

			}
		}
		if(waitting_for_delete){
			frame_cache_.erase(it++);
			BuffCollection(waitting_for_delete);
			delete waitting_for_delete;
		}else{
			break;
		}
	}
}
//true means not to deliver frame
bool MultipathReceiver::CheckLateFrame(uint32_t fid){
	bool ret=false;
	if(frame_cache_.empty()){
	}else{
		auto it=frame_cache_.begin();
		uint8_t min_fid=it->first;
		if(fid<min_fid){
		NS_LOG_WARN("late come frame");
			ret=true;
		}
	}
	return ret;
}
//waitting or not waitting, the retransmission packet ,
//that's a hard question.
//waitting_ts=ts+max(path_i_waitting)
//path_i_waitting=s_send_ts+rtt+rtt_var
// fid=1,waitting=-1,must to get the full frame
static uint32_t MAX_WAITTING_RETRANS_TIME=500;//500ms
static uint32_t PROTECTION_TIME=50;//50ms caution, relate to pace buffer threshold
uint32_t MultipathReceiver::GetWaittingDuration(){
	uint32_t path_waitting=0;
	uint32_t temp;
	PathInfo *path=NULL;
	for(auto it=paths_.begin();it!=paths_.end();it++){
		path=it->second;
		temp=path->rtt_+path->rtt_var_+path->s_sent_ts_;
		if(temp>path_waitting){
			path_waitting=temp;
		}
	}
	path_waitting+=PROTECTION_TIME;
	path_waitting=SU_MIN(path_waitting,MAX_WAITTING_RETRANS_TIME);
	return path_waitting;
}
// implement the most simple way retransmission waititng time;
void MultipathReceiver::DeliverToCache(uint8_t pid,sim_segment_t* d){
	video_packet_t *packet=NULL;
	uint32_t fid=d->fid;
	uint8_t ftype=d->ftype;
	uint16_t total=d->total;
	uint16_t index=d->index;
	if(CheckLateFrame(fid)){
		return;
	}
	video_frame_t *frame=NULL;
	{
		auto it=frame_cache_.find(fid);
		if(it!=frame_cache_.end()){
			frame=it->second;
		}else{
			frame=new video_frame_t();
            frame->fid=fid;
			frame->packets=(video_packet_t**)calloc(total,sizeof(sim_segment_t*));
			frame->recv=0;
			frame->total=total;
			frame->waitting_ts=0;
			frame->frame_type=ftype;
			if(ftype==1){
				frame->waitting_ts=-1;
			}
		}
	}
	if(!free_segs_.empty()){
		packet=free_segs_.front();
		free_segs_.pop();
	}else{
		packet=new video_packet_t();
		seg_c_++;
	}
	uint32_t now=rtc::TimeMillis();
	packet->pid=pid;
	packet->ts=now;
	sim_segment_t *seg=&(packet->seg);
	memcpy(seg,d,sizeof(sim_segment_t));
	frame->ts=now;
	frame->recv++;
	frame->packets[index]=packet;
	frame_cache_.insert(std::make_pair(fid,frame));
	if(frame->recv<frame->total&&frame->waitting_ts!=-1){
		frame->waitting_ts=now+GetWaittingDuration();
	}
	CheckDeliverFrame();
}
void MultipathReceiver::ProcessPongMsg(uint8_t pid,uint32_t rtt){
    if(stop_){return ;}
	PathInfo *path=GetPath(pid);
	path->UpdateRtt(rtt);
	razor_receiver_t *cc=NULL;
	cc=path->GetController()->r_cc_;
	if(cc){
		cc->update_rtt(cc,path->rtt_+path->rtt_var_);
	}
}
void MultipathReceiver::SendConnectAck(PathInfo *path,uint32_t cid){
    char ip_port[128]={0};
    su_addr_to_string(&path->dst,ip_port,128);
    NS_LOG_INFO("send ack to "<<ip_port);
	sim_header_t h;
	sim_connect_ack_t ack;
	INIT_SIM_HEADER(h, SIM_CONNECT_ACK,uid_);
	h.ver=path->pid;
	ack.cid = cid;
	ack.result = 0;
	sim_encode_msg(&strm_, &h, &ack);
	su_udp_send(path->fd,&path->dst,strm_.data,strm_.used);
}
void MultipathReceiver::ConfigureController(PathInfo *path,uint8_t transport_type){
	razor_receiver_t *cc=NULL;
	CongestionController *control=new CongestionController(this,path->pid);
	uint8_t cc_type=(transport_type==bbr_transport)?bbr_congestion:gcc_congestion;
	cc=razor_receiver_create(cc_type,min_bitrate_,max_bitrate_,SIM_SEGMENT_HEADER_SIZE,(void*)control,
			&MultipathReceiver::SendFeedBack);
	control->SetCongestion(cc,ROLE::ROLE_RECEIVER);
    path->SetController(control);
}
void MultipathReceiver::SendFeedBack(void* handler, const uint8_t* payload, int payload_size){
	CongestionController *control=static_cast<CongestionController*>(handler);
	MultipathReceiver *receiver=static_cast<MultipathReceiver*>(control->session_);
	uint8_t pid=control->pid_;
	PathInfo *path=receiver->GetPath(pid);
	sim_header_t header;
	sim_feedback_t feedback;
	if (payload_size > SIM_FEEDBACK_SIZE){
		NS_LOG_ERROR("feedback size > SIM_FEEDBACK_SIZE");
		return;
	}
	if(!path){
		return;
	}
	INIT_SIM_HEADER(header, SIM_FEEDBACK, receiver->uid_);
    header.ver=pid;
	feedback.base_packet_id =path->base_seq_;
	feedback.feedback_size = payload_size;
	memcpy(feedback.feedback, payload, payload_size);
	sim_encode_msg(&receiver->strm_, &header, &feedback);
	su_udp_send(path->fd,&path->dst,receiver->strm_.data,receiver->strm_.used);
}
static uint32_t PING_INTERVAL=250;
void MultipathReceiver::Process(){
    if(stop_){return ;}
	uint32_t now=rtc::TimeMillis();
	PathInfo *path=NULL;
	for(auto it=paths_.begin();it!=paths_.end();it++){
		path=it->second;
		path->ReceiverHeartBeat(now);
		if((now-path->rtt_update_ts_)>PING_INTERVAL){
			SendPingMsg(path,now);
		}
	}

}
void MultipathReceiver::Stop(){
	PathInfo *path=NULL;
	stop_=true;
	for(auto it=paths_.begin();it!=paths_.end();it++){
		path=it->second;
		path->ReceiverStop();
	}
}
}
