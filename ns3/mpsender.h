#ifndef SIM_TRANSPORT_MPSENDER_H_
#define SIM_TRANSPORT_MPSENDER_H_
#include <vector>
#include <map>
#include <queue>
#include "mpcommon.h"//for webrtc linux defination
#include "rtc_base/criticalsection.h"
#include "sessioninterface.h"
#include "schedule.h"
#include "ratecontrol.h"
#include "pathsender.h"
#include "ns3/videosource.h"
namespace ns3{
class MultipathSender:public SenderInterface{
public:
	MultipathSender(uint32_t uid);
	~MultipathSender();
	void Process();
	void Stop();
	void Connect(std::vector<InetSocketAddress> addr);
	void AddAvailablePath(uint8_t pid);
	void SendVideo(uint8_t payload_type,int ftype,
			void *data,uint32_t size) override;
	void OnNetworkChanged(uint8_t pid,
						  uint32_t bitrate_bps,
						  uint8_t fraction_loss,
						  int64_t rtt_ms) override;
	uint32_t GetUid() override {return uid_;}
	void PacketSchedule(uint32_t,uint8_t) override;
	void SetSchedule(Schedule*) override;
	void SetRateControl(RateControl *) override;
	int64_t GetFirstTs() override {return first_ts_;}
	void SetBitrate(uint32_t min,uint32_t start,uint32_t max){
		min_bitrate_=min;
		start_bitrate_=start;
		max_bitrate_=max;
	}

	void RegisterPath(Ptr<PathSender> path);
	void NotifyPathUsable(uint8_t pid) override;
	Ptr<PathSender> GetPathInfo(uint8_t) override;
    void RegisterPacketSource(VideoSource *v) override{source_=v;}
    void StopCallback() override;
private:
	void SchedulePendingBuf();
	std::map<uint8_t,Ptr<PathSender>> syn_path_;
	std::map<uint8_t,Ptr<PathSender>> usable_path_;
	uint32_t uid_;
//not to mark path id but to allocate path id
	uint8_t pid_;
	uint32_t frame_seed_;
	uint32_t schedule_seed_;
	std::map<uint32_t,sim_segment_t*>pending_buf_;
	uint32_t seg_c_;
	int64_t first_ts_;
	uint32_t min_bitrate_;
	uint32_t start_bitrate_;
	uint32_t max_bitrate_;
	Schedule *scheduler_;
	RateControl *rate_control_;
	bool stop_;
	SimulationClock clock_;
    VideoSource *source_;
};
}




#endif /* SIM_TRANSPORT_MPSENDER_H_ */
