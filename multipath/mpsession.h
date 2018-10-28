#ifndef __MP_SESSION_H_
#define __MP_SESSION_H_
#include <map>
#include <vector>
#include "rtc_base/thread.h"
#include "cf_stream.h"
#include "sim_external.h"
#include "sessioninterface.h"
#include "mpsender.h"
#include "mpreceiver.h"
namespace zsy{
class MultipathSession:public rtc::Thread,SessionInterface{
public:
	MultipathSession(int port,uint32_t uid);
	~MultipathSession();
	void RegisterCallback(sim_notify_fn notify,void *arg);
	// at first create sender;
	MultipathSender *CreateSender();
	void Connect(int num,...);
	void Disconnect();
	void Start();
	void Stop();
	void Run() override;
	void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t len);
	bool Fd2Addr(su_socket fd,su_addr *addr) override;
	void PathStateForward(int type,int value) override;
private:
	void ProcessingMsg(su_socket *fd,su_addr *remote,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt);
	void SendDisconMsg();
	void StopSession();
	int num_;
	su_addr *addr_pair_;
	uint32_t uid_; //odd value;
	std::map<uint8_t,su_socket> p2fd_;
	std::vector<su_socket>  fds_;
	std::map<su_socket,su_addr> fd2addr_;
	MultipathSender *sender_;
	MultipathReceiver *receiver_;
	bool running_;
	bool stop_;
	bool recv_dis_;
	bool send_dis_;
	uint32_t next_send_dis_ts_;
	uint32_t send_dis_c_;
	sim_notify_fn notify_cb_;
	void *notify_arg_;
};
}
#endif
