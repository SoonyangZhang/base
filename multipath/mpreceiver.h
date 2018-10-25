#ifndef SIM_TRANSPORT_MPRECEIVER_H_
#define SIM_TRANSPORT_MPRECEIVER_H_
#include <map>
#include<queue>
#include "rtc_base/criticalsection.h"
#include "sessioninterface.h"
#include "path.h"
namespace zsy{
class MultipathReceiver{
public:
	MultipathReceiver(SessionInterface*session,uint32_t uid);
	~MultipathReceiver();
	void Process();
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt);
	static void SendFeedBack(void* handler, const uint8_t* payload, int payload_size);
private:
	void ProcessSegMsg(uint8_t,sim_segment_t*);
	void SendConnectAck(PathInfo *path,uint32_t cid);
	void ConfigureController(PathInfo *path,uint8_t transport_type);
	uint32_t uid_;
	uint32_t min_bitrate_;
	uint32_t max_bitrate_;
	SessionInterface *session_;
	rtc::CriticalSection free_mutex_;
	std::queue<sim_segment_t *>free_segs_;
	uint32_t seg_c_;
	std::map<uint8_t,PathInfo*> paths_;
	bin_stream_t	strm_;
};
}
#endif /* SIM_TRANSPORT_MPRECEIVER_H_ */
