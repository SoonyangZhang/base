#ifndef __MP_SESSION_H_
#define __MP_SESSION_H_
#include <map>
#include <vector>
#include "rtc_base/thread.h"
#include "rtc_base/criticalsection.h"
#include "cf_stream.h"
#include "sessioninterface.h"
#include "mpsender.h"
#include "mpreceiver.h"
#include "ratecontrol.h"
namespace zsy{
enum MessageType{
	message_send_dis,
};
struct SessionMessage{
    uint32_t type;
    uint32_t value;
};
class MultipathSession:public rtc::Thread, public SessionInterface{
public:
	MultipathSession(int port,uint32_t uid);
	~MultipathSession();
	void RegisterCallback(sim_notify_fn notify,void *arg);
	// at first create sender;
	void CreateSender();
	void RegisterRateControl(RateControl *rate);
	void RegistePacketSchedule(Schedule *schedule);
	bool RegisterConsumer(NetworkDataConsumer*c);
	void Connect(int num,...);
	void Start();
	void Stop();
	void Run() override;
	void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t len);
	bool Fd2Addr(su_socket fd,su_addr *addr) override;
	void PathStateForward(int type,int value) override;
	void PostMessage(SessionMessage&msg);
private:
	void HandleMessage();
	void Disconnect();
    void StopSession();
	void ProcessingMsg(su_socket *fd,su_addr *remote,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt,uint32_t now);
	void SendDisconMsg();
	int num_;
	su_addr *addr_pair_;
	uint32_t uid_; //odd value;
	std::map<uint8_t,su_socket> p2fd_;
	std::vector<su_socket>  fds_;
	std::map<su_socket,su_addr> fd2addr_;
    rtc::CriticalSection message_lock_;
    std::list<SessionMessage> message_;
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
