#ifndef SIM_TRANSPORT_MPRECEIVER_H_
#define SIM_TRANSPORT_MPRECEIVER_H_
#include <map>
#include<queue>
#include "sessioninterface.h"
#include "pathreceiver.h"
#include "ns3/simulationclock.h"
#include "ns3/event-id.h"
namespace ns3{
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
	MultipathReceiver(uint32_t uid);
	~MultipathReceiver();
	void HeartBeat();
	void Process();
	void Stop();
	bool RegisterDataSink(NetworkDataConsumer* c);
    void DeliverToCache(uint8_t pid,sim_segment_t* d) override; 
    uint32_t GetUid() override{return uid_;}
	void RegisterPath(Ptr<PathReceiver> path);
    void StopCallback() override;
private:
	Ptr<PathReceiver> GetPathInfo(uint8_t);
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
	rtc::CriticalSection free_mutex_;
	std::queue<video_packet_t *>free_segs_;
	uint32_t seg_c_;
	std::list<Ptr<PathReceiver>> paths_;
	bin_stream_t	strm_;
	std::map<uint32_t,video_frame_t*> frame_cache_;
	NetworkDataConsumer *deliver_;
	bool stop_;
	uint32_t last_deliver_ts_;
	uint32_t min_fid_;//record the fid that has deliverd to uplayer;
	SimulationClock clock_;
	bool first_packet_;
	EventId hbTimer_;
};
}
#endif /* SIM_TRANSPORT_MPRECEIVER_H_ */
