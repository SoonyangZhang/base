#ifndef SIM_TRANSPORT_MPRECEIVER_H_
#define SIM_TRANSPORT_MPRECEIVER_H_
#include <map>
#include<queue>
#include "rtc_base/criticalsection.h"
#include "modules/utility/source/process_thread_impl.h"
#include "sessioninterface.h"
#include "pathreceiver.h"
#include "emulationclock.h"
namespace zsy{
struct video_packet_t{
	uint8_t pid;
	sim_segment_t seg;
};
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
class MultipathReceiver :public ReceiverInterface{
public:
	MultipathReceiver(SessionInterface*session,uint32_t uid);
	~MultipathReceiver();
	void Process();
	void Stop();
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt,uint32_t now);
	bool RegisterDataSink(NetworkDataConsumer* c);
    void DeliverToCache(uint8_t pid,sim_segment_t* d) override; 
    uint32_t GetUid() override{return uid_;}
private:
	PathReceiver *GetPathInfo(uint8_t);
	void ProcessSegMsg(sim_header_t*,sim_segment_t*);
	bool DeliverFrame(video_frame_t *f);
	void PacketConsumed(video_frame_t *f);
	void BuffCollection(video_frame_t*f);
	void CheckDeliverFrame();
	bool CheckLateFrame(uint32_t fid);
	uint32_t GetWaittingDuration();
	void UpdateDeliverTime(uint32_t ts);
	void CheckFrameDeliverBlock(uint32_t);
	uint32_t uid_;
	uint32_t min_bitrate_;
	uint32_t max_bitrate_;
	SessionInterface *session_;
	rtc::CriticalSection free_mutex_;
	std::queue<video_packet_t *>free_segs_;
	uint32_t seg_c_;
	std::map<uint8_t,PathReceiver*> paths_;
	bin_stream_t	strm_;
	std::map<uint32_t,video_frame_t*> frame_cache_;
	NetworkDataConsumer *deliver_;
	bool stop_;
	uint32_t last_deliver_ts_;
	uint32_t min_fid_;//record the fid that has deliverd to uplayer;
	EmulationClock clock_;
	webrtc::ProcessThreadImpl *pm_;
};
}
#endif /* SIM_TRANSPORT_MPRECEIVER_H_ */
