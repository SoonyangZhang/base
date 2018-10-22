#include "mpsender.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include <string>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathSender");
static uint32_t CONN_TIMEOUT=200000;//200ms
static uint32_t CONN_RETRY=3;
SenderBuf::SenderBuf(uint8_t p)
:pid_(p){

}
SenderBuf::~SenderBuf(){
//TODO buf
}
bool SenderBuf::put(sim_segment_t*seg){
	uint32_t id=seg->packet_id;
	auto it=buf_.find(id);
	if(it!=buf_.end()){
		return false;
	}
	len_+=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
	buf_.insert(std::make_pair(id,seg));
	return true;
}
sim_segment_t *SenderBuf::get_segment(uint32_t id){
	sim_segment_t *seg=NULL;
	auto it=buf_.find(id);
	if(it!=buf_.end()){
		seg=it->second;
		len_-=(seg->data_size+SIM_SEGMENT_HEADER_SIZE);
		buf_.erase(it);
	}
    return seg;
}
uint32_t SenderBuf::get_delay(){
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
uint32_t SenderBuf::get_len(){
	return len_;
}
MultipathSender::MultipathSender(SessionInterface *session,uint32_t uid)
:uid_{uid},pid_{1}
,session_(session),frame_seed_{1}{
	bin_stream_init(&strm_);
}
MultipathSender::~MultipathSender(){
	bin_stream_destroy(&strm_);
}
void MultipathSender::Drive(){
   // NS_LOG_INFO("drive");
	if(!syn_path_.empty()){
		uint32_t now=rtc::TimeMillis();
		for(auto it=syn_path_.begin();it!=syn_path_.end();){
			if(it->con_c>CONN_RETRY){
				it=syn_path_.erase(it);
				uint8_t pid=it->pid;
				NS_LOG_INFO("path"<<pid<<"is unreachable");
			}else{
				uint32_t delta=now-it->ts;
				uint32_t backoff=it->con_c*CONN_TIMEOUT;
				if(delta>backoff){
					it->con_c++;
					it->ts=now;
					SendConnect(*it,now);
				}
				it++;
			}
		}
	}
}
void MultipathSender::Connect(PathInfo p){
	PathInfo path=p;
	uint32_t now=rtc::TimeMillis();
	path.state=path_conning;
	path.pid=pid_;
	pid_++;
	path.ts=now;
	path.con_c=1;
    SendConnect(path,now);
	syn_path_.push_back(path);
}
void MultipathSender::SendConnect(PathInfo &p,uint32_t now){
    NS_LOG_INFO("send con");
	sim_header_t header;
	sim_connect_t body;
	INIT_SIM_HEADER(header, SIM_CONNECT,uid_);
	//use protocol var to mark path id
	header.ver=p.pid;
	//use cid to compute rtt
	body.cid =now ;
	body.token_size = 0;
	body.cc_type =gcc_transport;
	bin_stream_rewind(&strm_,1);
	sim_encode_msg(&strm_,&header,&body);
    NS_LOG_INFO("send con size "<<strm_.used);
	su_udp_send(p.fd,&p.dst,strm_.data, strm_.used);
}
void MultipathSender::ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream){
	switch(header->mid){
	case SIM_CONNECT_ACK:{
		sim_connect_ack_t ack;
		if (sim_decode_msg(stream, header, &ack) != 0)
			return;
		uint8_t pid=header->ver;
        printf("pid %d\n",pid);
		//TODO cid can be use to updata rtt
		for(auto it=syn_path_.begin();it!=syn_path_.end();it++){
			if(it->pid==pid){
				PathInfo path=*it;
				path.state=path_conned;
				syn_path_.erase(it);
				usable_paths_.push_back(path);
				AddPath(pid);
				NS_LOG_INFO("path "<<std::to_string(pid)<< "is usable");
				break;
			}
		}
	}
	}
}
void MultipathSender::AddPath(uint8_t p){
	razor_sender_t *sender;
	int cc_type=gcc_congestion;
	int padding=1;
	auto it=controllers_.find(p);
	if(it==controllers_.end()){
		CongestionController *controller=new CongestionController(this,p);
		sender=razor_sender_create(cc_type,padding,
		(void*)controller,&MultipathSender::ChangeRate,
		(void*)controller,&MultipathSender::PaceSend,1000);
		controller->SetCongestion((void*)sender,zsy::ROLE::ROLE_SENDER);
		controllers_.insert(std::make_pair(p,controller));
	}

}
void MultipathSender::SendVideo(int ftype,void *data,uint32_t len){
// may implement a packet scheduler;
}
void MultipathSender::ChangeRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt){

}
void MultipathSender::PaceSend(void * handler, uint32_t packet_id, int retrans, size_t size, int padding){

}
}
