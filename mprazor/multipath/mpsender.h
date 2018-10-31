#ifndef SIM_TRANSPORT_MPSENDER_H_
#define SIM_TRANSPORT_MPSENDER_H_
#include <vector>
#include <map>
#include <queue>
#include "rtc_base/criticalsection.h"
#include "razor_api.h"
#include "sessioninterface.h"
#include "schedule.h"
#include "ratecontrol.h"
namespace zsy{
class MultipathSender:public SenderInterface{
public:
	MultipathSender(SessionInterface *session,uint32_t uid);
	~MultipathSender();
	void Process();
	void Stop();
	void Connect(su_socket *fd,su_addr *src,su_addr *dst);
	void SendDisconnect(PathInfo *path,uint32_t ts);
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
	void ProcessPongMsg(uint8_t pid,uint32_t rtt);
	void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t size);
	static void ChangeRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt);
	static void PaceSend(void* handler, uint32_t packet_id, int retrans, size_t size, int padding);
	PathInfo* GetPathInfo(uint8_t) override;
	uint32_t GetUid() override{return uid_;}
	PathInfo *GetMinRttPath();
    void AddAvailablePath(PathInfo*path);
	void PacketSchedule(uint32_t,uint8_t) override;
	void SetSchedule(Schedule*);
	void SetRateControl(RateControl *);
	int64_t GetFirstTs() override {return first_ts_;}
	void Reclaim(sim_segment_t *seg) override;
	void SetCongestionType(uint8_t cc_type){cc_type_=cc_type;}
	void SetBitrate(uint32_t min,uint32_t start,uint32_t max){
		min_bitrate_=min;
		start_bitrate_=start;
		max_bitrate_=max;
	}
private:
	void SendConnect(PathInfo *p,uint32_t now);
	void SendPingMsg(PathInfo *path,uint32_t now);
	void AddController(PathInfo*);
	void SchedulePendingBuf();
	void InnerProcessFeedback(uint8_t,sim_feedback_t*);
	void InnerProcessSegAck(uint8_t,sim_segment_ack_t*);
	std::map<uint8_t,PathInfo*> usable_path_;
	std::vector<PathInfo*> syn_path_;
	std::vector<PathInfo*> dis_path_;
	uint32_t uid_;
//not to mark path id but to allocate path id
	uint8_t pid_;
	bin_stream_t	strm_;
	SessionInterface *session_;
	uint32_t frame_seed_;
	uint32_t schedule_seed_;
	//get segment from path and put it to sent_buf_
	//for retransmission or waitting for delete;
	//as for path initial phase,may generate some padding packet;
	rtc::CriticalSection buf_mutex_;
	std::map<uint32_t,sim_segment_t*>pending_buf_;
	rtc::CriticalSection free_mutex_;
	std::queue<sim_segment_t *>free_segs_;
	uint32_t seg_c_;
	int64_t first_ts_;
	uint8_t cc_type_;
	uint32_t min_bitrate_;
	uint32_t start_bitrate_;
	uint32_t max_bitrate_;
	Schedule *scheduler_;
	RateControl *rate_control_;
	bool stop_;
	uint32_t max_free_buf_th_;
};
}




#endif /* SIM_TRANSPORT_MPSENDER_H_ */
