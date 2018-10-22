#ifndef SIM_TRANSPORT_MPSENDER_H_
#define SIM_TRANSPORT_MPSENDER_H_
#include "razor_api.h"
#include "sessioninterface.h"
#include <vector>
#include <map>
#include <queue>
namespace zsy{
class SenderBuf{
public:
	SenderBuf(uint8_t p);
	~SenderBuf();
	bool put(sim_segment_t*seg);
	sim_segment_t *get_segment(uint32_t);
	uint32_t get_delay();
	uint32_t get_len();
private:
	uint8_t pid_;
	std::map<uint32_t,sim_segment_t*> buf_;
	uint32_t len_;//byte
};
class MultipathSender{
public:
	MultipathSender(SessionInterface *session,uint32_t uid);
	~MultipathSender();
	void Drive();
	void Connect(PathInfo p);
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
	void SendVideo(int ftype,void *data,uint32_t len);
	static void ChangeRate(void* trigger, uint32_t bitrate, uint8_t fraction_loss, uint32_t rtt);
	static void PaceSend(void* handler, uint32_t packet_id, int retrans, size_t size, int padding);
private:
	void SendConnect(PathInfo &p,uint32_t now);
	void AddPath(uint8_t p);
	std::vector<PathInfo> usable_paths_;
	std::vector<PathInfo> syn_path_;
	std::vector<PathInfo> dis_path_;
	std::map<uint8_t,CongestionController*>controllers_;
	uint32_t uid_;
	uint8_t pid_;
	bin_stream_t	strm_;
	SessionInterface *session_;
	uint32_t frame_seed_;
	// path->sement for rescheduling
	std::map<uint32_t,uint8_t> seg2path_;
	std::map<uint8_t,SenderBuf*>path_buf_;
	//get segment from path and put it to sent_buf_
	//for retransmisson or waititng for delete;
	std::map<uint32_t,sim_segment_t*> sent_buf_;
	std::map<uint32_t,sim_segment_t*>send_buf_;
	std::queue<sim_segment_t *>free_segs_;
};
}




#endif /* SIM_TRANSPORT_MPSENDER_H_ */
