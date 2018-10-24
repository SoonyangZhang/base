#ifndef SIM_TRANSPORT_MPSENDER_H_
#define SIM_TRANSPORT_MPSENDER_H_
#include <vector>
#include <map>
#include <queue>
#include "rtc_base/criticalsection.h"
#include "razor_api.h"
#include "sessioninterface.h"
#include "schedule.h"

namespace zsy{
class MultipathSender:public SenderInterface{
public:
	MultipathSender(SessionInterface *session,uint32_t uid);
	~MultipathSender();
	void Drive();
	void Connect(su_socket *fd,su_addr *src,su_addr *dst);
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
	void SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t size);
	static void ChangeRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt);
	static void PaceSend(void* handler, uint32_t packet_id, int retrans, size_t size, int padding);
	PathInfo* GetPathInfo(uint8_t) override;
	void PacketSchedule(uint32_t,uint8_t) override;
	void SetSchedule(Schedule*);
	int64_t GetFirstTs() override{return first_ts_;}
private:
	void SendConnect(PathInfo *p,uint32_t now);
	void AddController(PathInfo*);
	void SchedulePendingBuf();
	std::map<uint8_t,PathInfo*> usable_path_;
	std::vector<PathInfo*> syn_path_;
	std::vector<PathInfo*> dis_path_;
	uint32_t uid_;
	uint8_t pid_;
	bin_stream_t	strm_;
	SessionInterface *session_;
	uint32_t frame_seed_;
	uint32_t packet_seed_;
	// path->segment for rescheduling
	std::map<uint32_t,uint8_t> seg2path_;
	//get segment from path and put it to sent_buf_
	//for retransmission or waitting for delete;
	//as for path initial phase,may generate some padding packet;
	std::map<uint32_t,sim_segment_t*> sent_buf_;
	rtc::CriticalSection buf_mutex_;
	std::map<uint32_t,sim_segment_t*>pending_buf_;
	rtc::CriticalSection free_mutex_;
	std::queue<sim_segment_t *>free_segs_;
	uint32_t seg_c_;
	int64_t first_ts_;
	Schedule *scheduler_;
};
}




#endif /* SIM_TRANSPORT_MPSENDER_H_ */
