#include "mpreceiver.h"
#include "log.h"
#include <stdio.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");
MultipathReceiver::MultipathReceiver(SessionInterface*session,uint32_t uid)
:uid_(uid)
,session_(session){
	bin_stream_init(&strm_);
}
MultipathReceiver::~MultipathReceiver(){
	bin_stream_destroy(&strm_);
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
		PathInfo path;
		path.fd=*fd;
		path.dst=*remote;
		session_->Fd2Addr(*fd,&path.src);
		path.pid=header->ver;
        printf("path id %d uid%d\n ",path.pid,header->uid);
		path.state=path_conned;
		auto it=paths_.find(path.pid);
		if(it!=paths_.end()){
			paths_.erase(it);
		}
		paths_.insert(std::make_pair(path.pid,path));
		SendConnectAck(path,cid);
        break;
	}
	}
}
void MultipathReceiver::SendConnectAck(PathInfo &path,uint32_t cid){
    char ip_port[128]={0};
    su_addr_to_string(&path.dst,ip_port,128);
    NS_LOG_INFO("send ack to "<<ip_port);
	sim_header_t h;
	sim_connect_ack_t ack;
	INIT_SIM_HEADER(h, SIM_CONNECT_ACK,uid_);
	h.ver=path.pid;
	ack.cid = cid;
	ack.result = 0;
	sim_encode_msg(&strm_, &h, &ack);
	su_udp_send(path.fd,&path.dst,strm_.data,strm_.used);
}
void MultipathReceiver::Drive(){

}
}




