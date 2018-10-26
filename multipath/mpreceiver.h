#ifndef SIM_TRANSPORT_MPRECEIVER_H_
#define SIM_TRANSPORT_MPRECEIVER_H_
#include <map>
#include<queue>
#include "rtc_base/criticalsection.h"
#include "sessioninterface.h"
#include "path.h"
namespace zsy{
struct video_packet_t{
	uint8_t pid;
	uint32_t ts;
	sim_segment_t seg;
};
//waitting or not waitting, the retransmission packet ,
//that's a hard question.
//waitting_ts=ts+max(path_i_waitting)
//path_i_waitting=s_send_ts+rtt+rtt_var
// fid=1,waittinh=-1,must to get the full frame
struct video_frame_t{
	uint32_t fid;
	uint32_t total;
	uint32_t recv;
	uint32_t ts;//  newest timestamp;
	int64_t waitting_ts;//waitting for lost packet
	//the max time to deliver packet
	int frame_type;
	video_packet_t **packets;
};
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
	bool CheckLateFrame(uint32_t fid);
	void DeliverToCache(uint8_t pid,sim_segment_t* d);
	uint32_t uid_;
	uint32_t min_bitrate_;
	uint32_t max_bitrate_;
	SessionInterface *session_;
	rtc::CriticalSection free_mutex_;
	std::queue<video_packet_t *>free_segs_;
	uint32_t seg_c_;
	std::map<uint8_t,PathInfo*> paths_;
	bin_stream_t	strm_;
	std::map<uint32_t,video_frame_t*> frame_cache_;
};
}
#endif /* SIM_TRANSPORT_MPRECEIVER_H_ */
