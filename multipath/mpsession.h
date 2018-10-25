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
	// at first create sender;
	MultipathSender *CreateSender();
	void Connect(int num,...);
	void Start();
	void Stop();
	void Run() override;
	void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t len);
	bool Fd2Addr(su_socket fd,su_addr *addr) override;
	void PathStateForward() override;
private:
	void ProcessingMsg(su_socket *fd,su_addr *remote,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt);
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
	sim_notify_fn notify_cb_;
	void *nitify_arg_;
};
}
#endif
