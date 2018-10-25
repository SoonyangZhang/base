#include "mpreceiver.h"
#include "sim_external.h"
#include "log.h"
#include <stdio.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");
MultipathReceiver::MultipathReceiver(SessionInterface*session,uint32_t uid)
:uid_(uid)
,min_bitrate_(MIN_BITRATE)
,max_bitrate_(MAX_BITRATE)
,session_(session)
,seg_c_(0){
	bin_stream_init(&strm_);
	for(int i=0;i<2;i++){
		sim_segment_t *seg=new sim_segment_t();
		free_segs_.push(seg);
		seg_c_++;
	}
}
MultipathReceiver::~MultipathReceiver(){
	bin_stream_destroy(&strm_);
	sim_segment_t *seg=NULL;
	while(!free_segs_.empty()){
		seg=free_segs_.front();
		free_segs_.pop();
		seg_c_--;
		delete seg;
	}
}
void MultipathReceiver::ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header
		,bin_stream_t *stream){
	switch (header->mid){
	case SIM_CONNECT:
	{
 		sim_connect_t body;
		if (sim_decode_msg(stream, header, &body) != 0)
			return;
		uint32_t cid=body.cid;
		uint8_t cc_type=body.cc_type;
		uint8_t pid=header->ver;
		PathInfo *path=NULL;
		auto it=paths_.find(pid);
		if(it!=paths_.end()){
			path=it->second;
		}else{
			path=new PathInfo();
			path->pid=pid;
			path->fd=*fd;
			path->dst=*remote;
			path->state=path_conned;
			session_->Fd2Addr(*fd,&path->src);
            paths_.insert(std::make_pair(pid,path));
			ConfigureController(path,cc_type);		
		}
        printf("path id %d uid%d\n ",path->pid,header->uid);
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
		PathInfo *path=NULL;
		sim_pad_t pad;
		if (sim_decode_msg(stream, header, &pad) != 0)
			return;
		auto it=paths_.find(pid);
		if(it!=paths_.end()){
			path=it->second;
		}
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
void MultipathReceiver::ProcessSegMsg(uint8_t pid,sim_segment_t*data){
	sim_segment_t *seg=NULL;
	PathInfo *path=NULL;
	{
		auto it=paths_.find(pid);
		if(it!=paths_.end()){
			path=it->second;
		}
	}
	if(path==NULL){
		return;
	}
	if(!free_segs_.empty()){
		seg=free_segs_.front();
		free_segs_.pop();
	}else{
		seg=new sim_segment_t();
		seg_c_++;
	}
	memcpy(seg,data,sizeof(sim_segment_t));
	uint32_t packet_id=seg->packet_id;
	razor_receiver_t *cc=NULL;
	cc=path->GetController()->r_cc_;
	if(cc){
		cc->on_received(cc,seg->transport_seq,seg->timestamp + seg->send_ts,
				seg->data_size + SIM_SEGMENT_HEADER_SIZE, seg->remb);
	}
}
void MultipathReceiver::ProcessPongMsg(uint8_t pid,uint32_t rtt){

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
	PathInfo *path=NULL;
	auto it=receiver->paths_.find(pid);
	if(it==receiver->paths_.end()){
		NS_LOG_ERROR("path not foud");
		return;
	}
	path=it->second;
	sim_header_t header;
	sim_feedback_t feedback;
	if (payload_size > SIM_FEEDBACK_SIZE){
		NS_LOG_ERROR("feedback size > SIM_FEEDBACK_SIZE");
		return;
	}
	INIT_SIM_HEADER(header, SIM_FEEDBACK, receiver->uid_);
	feedback.base_packet_id =path->base_seq_;
	feedback.feedback_size = payload_size;
	memcpy(feedback.feedback, payload, payload_size);
	sim_encode_msg(&receiver->strm_, &header, &feedback);
	su_udp_send(path->fd,&path->dst,receiver->strm_.data,receiver->strm_.used);
}
void MultipathReceiver::Process(){

}
}




