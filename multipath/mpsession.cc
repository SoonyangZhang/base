#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string>
#include<stdio.h>
#include "rtc_base/timeutils.h"
#include "mpsession.h"
#include "log.h"
typedef std::map<std::string,std::string> settings_t;
static std::string localAddr("127.0.0.1");
void loadLocalIp (settings_t &ipConfig)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            std::string key(ifa->ifa_name);
            std::string value(addressBuffer);
            ipConfig.insert(std::pair<std::string,std::string>(key, value));
         }
     }
    if (ifAddrStruct!=NULL)
        freeifaddrs(ifAddrStruct);//remember to free ifAddrStruct
}
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathSession");
MultipathSession::MultipathSession(int port,uint32_t uid)
:num_(0)
,addr_pair_(NULL)
,uid_(uid)
,sender_(NULL)
,receiver_(NULL)
,running_{false}
,stop_{false}
,recv_dis_{false}
,send_dis_{false}
,next_send_dis_ts_{0}
,send_dis_c_{0}
,notify_cb_{NULL}
,notify_arg_{NULL}{
	settings_t ipAddrs;
	loadLocalIp(ipAddrs);
	for(auto it=ipAddrs.begin();it!=ipAddrs.end();it++){
		std::string ip=it->second;
		if(ip.compare(localAddr)){
			su_socket fd;
			su_udp_create(ip.c_str(),port,&fd);
            NS_LOG_INFO("new fd "<<fd);
			su_udp_noblocking(fd);
			fds_.push_back(fd);
			su_addr addr;
			su_set_addr(&addr,ip.c_str(),port);
			fd2addr_.insert(std::make_pair(fd,addr));
		}
	}
}
MultipathSession::~MultipathSession(){
	if(!fds_.empty()){
		for(auto it=fds_.begin();it!=fds_.end();it++){
			su_socket fd=*it;
			su_socket_destroy(fd);
		}
	}
	if(sender_){
		delete sender_;
	}
	if(receiver_){
		delete receiver_;
	}
}
void MultipathSession::RegisterCallback(sim_notify_fn notify,void *arg){
	notify_cb_=notify;
	notify_arg_=arg;
}
MultipathSender* MultipathSession::CreateSender(){
	 sender_=new MultipathSender(this,uid_);
	 return sender_;
}
void MultipathSession::Connect(int num,...){
	num_=num;
	addr_pair_=new su_addr[num];
	if(num_%2!=0){return ;}
	int i=0;
	va_list args;
	va_start(args, num);
	su_addr addr;
	for(i=0;i<num_;i++){
		addr=va_arg(args, su_addr);
		addr_pair_[i]=addr;
	}
	va_end(args);
	for(i=0;i<num_;i+=2){
		for(auto it=fd2addr_.begin();it!=fd2addr_.end();it++){
			if(memcmp(&addr_pair_[i],&(it->second),sizeof(su_addr))==0){
                su_socket fd=it->first;
				su_addr src=it->second;
                su_addr dst=addr_pair_[i+1];
                if(sender_){
                	sender_->Connect(&fd,&src,&dst);
                }
			}
		}
	}
}
void MultipathSession::Disconnect(){
	send_dis_=true;
	SendDisconMsg();
}
void MultipathSession::SendDisconMsg(){
	uint32_t now=rtc::TimeMillis();
	uint32_t wait_time=0;
	if(sender_){
		PathInfo *path=sender_->GetMinRttPath();
		if(path){
			wait_time=path->rtt_+path->rtt_var_;
			sender_->SendDisconnect(path,now);
			send_dis_c_++;
			next_send_dis_ts_=now+send_dis_c_*wait_time;
		}
	}
}
void MultipathSession::StopSession(){
	if(sender_){

	}
	if(receiver_){

	}
}
void MultipathSession::Start(){
	if(!running_){
		running_=true;
		rtc::Thread::Start();
	}
}
void MultipathSession::Stop(){
	if(running_){
		running_=false;
		rtc::Thread::Stop();
	}
}
void MultipathSession::Run(){
	bin_stream_t rstrm;
	bin_stream_init(&rstrm);
	bin_stream_resize(&rstrm, 1500);
	while(running_){
		if(send_dis_){
			if(!recv_dis_){
				uint32_t now=rtc::TimeMillis();
				if(now>next_send_dis_ts_){
					SendDisconMsg();
				}
				if(send_dis_c_>3){
					recv_dis_=true;
					if(notify_cb_){
						notify_cb_(notify_arg_,NOTIFYMESSAGE::notify_dis,0);
					}
					StopSession();
				}
			}
		}
		if(!fds_.empty()&&!recv_dis_){
			if(sender_){
				sender_->Process();
			}
			if(receiver_){
				receiver_->Process();
			}
		for(auto it=fds_.begin();it!=fds_.end();it++){
			bin_stream_rewind(&rstrm, 1);
			su_socket fd=*it;
			su_addr remote;
			int ret=0;
            //NS_LOG_INFO("fd "<<fd);
			ret=su_udp_recv(fd,&remote,rstrm.data,rstrm.size,0);
            //NS_LOG_INFO("RECV SIZE "<<ret);
			if(ret>0){
                NS_LOG_INFO("RECV SIZE "<<ret);
                rstrm.used=ret;
				ProcessingMsg(&fd,&remote,&rstrm);
			}
		}
		}
	}
    bin_stream_destroy(&rstrm);
	stop_=true;
}
bool MultipathSession::Fd2Addr(su_socket fd,su_addr *addr){
	auto it=fd2addr_.find(fd);
	if(it!=fd2addr_.end()){
		*addr=it->second;
		return true;
	}
	return false;
}
void MultipathSession::ProcessingMsg(su_socket *fd,su_addr *remote,bin_stream_t *stream){
	sim_header_t header;
	if (sim_decode_header(stream, &header) != 0)
		return;

	if (header.mid < MIN_MSG_ID || header.mid > MAX_MSG_ID)
		return;
	uint8_t pid=header.ver;
	switch(header.mid){
	case SIM_CONNECT:
	{
        NS_LOG_INFO("recv conn");
		uint32_t peer_uid=header.uid;//may be used to create other receiver.
		if(!receiver_){
			receiver_=new MultipathReceiver(this,uid_);
		}
		receiver_->ProcessingMsg(fd,remote,&header,stream);
		break;
	}
	case SIM_CONNECT_ACK:{
        //NS_LOG_INFO("recv ack");
        if(!sender_){
        sender_=new MultipathSender(this,uid_);
        }
		sender_->ProcessingMsg(fd,remote,&header,stream);
        break;
	}
	case SIM_PONG:{
		sim_pong_t pong;
		sim_decode_msg(stream, &header, &pong);
		uint32_t rtt=rtc::TimeMillis();
		ProcessPongMsg(pid,rtt);
        break;
	}
	case SIM_PING:{
		sim_header_t h;
		sim_ping_t ping;
		sim_decode_msg(stream, &header, &ping);
		INIT_SIM_HEADER(h, SIM_PONG, uid_);
        h.ver=pid;
		sim_encode_msg(stream, &h, &ping);
		su_udp_send(*fd,remote,stream->data,stream->used);
        break;
	}
	case SIM_SEG:{
		if(receiver_){
			receiver_->ProcessingMsg(fd,remote,&header,stream);
		}
		break;
	}
	case SIM_SEG_ACK:{
		if(sender_){
			sender_->ProcessingMsg(fd,remote,&header,stream);
		}
		break;
	}
	case SIM_FEEDBACK:{
		if(sender_){
			sender_->ProcessingMsg(fd,remote,&header,stream);
		}
		break;
	}
	case SIM_FIR:{
		if(sender_){
			sender_->ProcessingMsg(fd,remote,&header,stream);
		}
		break;
	}
	case SIM_PAD:{
		if(receiver_){
			receiver_->ProcessingMsg(fd,remote,&header,stream);
		}
		break;
	}
	case SIM_DISCONNECT:{
		recv_dis_=true;
		sim_header_t h;
		sim_disconnect_t body;
		sim_disconnect_ack_t ack;

		if(sim_decode_msg(stream, &header, &body) != 0)
		return;
		INIT_SIM_HEADER(h, SIM_DISCONNECT_ACK, uid_);
		h.ver=pid;
		ack.cid = body.cid;
		ack.result = 0;
		sim_encode_msg(stream, &h, &ack);
		if(notify_cb_){
			notify_cb_(notify_arg_,NOTIFYMESSAGE::notify_dis,0);
		}
		StopSession();
		break;
	}
	case SIM_DISCONNECT_ACK:{
		recv_dis_=true;
		if(notify_cb_){
			notify_cb_(notify_arg_,NOTIFYMESSAGE::notify_dis_ack,0);
		}
		StopSession();
		break;
	}
    default :{
    NS_LOG_INFO("msg error");
    break;
    }
	}
}
void MultipathSession::ProcessPongMsg(uint8_t pid,uint32_t rtt){
	if(sender_){
		sender_->ProcessPongMsg(pid,rtt);
	}
	if(receiver_){
		receiver_->ProcessPongMsg(pid,rtt);
	}
}
void MultipathSession::PathStateForward(int type,int value){
	if(notify_cb_){
		notify_cb_(notify_arg_,type,value);
	}
}
void MultipathSession::SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t len){
	if(!recv_dis_&&!send_dis_){
		if(sender_){
			sender_->SendVideo(payload_type,ftype,data,len);
		}
	}
}
}
