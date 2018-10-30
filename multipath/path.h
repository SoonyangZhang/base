#ifndef MULTIPATH_PATH_H_
#define MULTIPATH_PATH_H_
#include "mpcommon.h"
#include "sessioninterface.h"
#include <map>
#include<set>
#include<list>
#include <queue>
namespace zsy{
struct send_buf_t{
	uint32_t ts;
	sim_segment_t *seg;
};
class PathInfo{
public:
PathInfo();
~PathInfo();
void SenderHeartBeat(uint32_t ts);
void ReceiverHeartBeat(uint32_t ts);
bool put(sim_segment_t*seg);
sim_segment_t *get_segment(uint32_t packet_id,int retrans,uint32_t ts);
uint32_t get_delay();
uint32_t get_len();
uint32_t GetSentBufSize(){return sent_buf_.size();}
void SetController(CongestionController*);
CongestionController* GetController();
void OnReceiveSegment(sim_segment_t *seg);
void SendToNetwork(uint8_t*data,uint32_t len);
void Consume(uint32_t packet_id);
void VideoRealAck(int hb,uint32_t seq);
send_buf_t * GetSentPacketInfo(uint32_t seq);
void RecvSegAck(sim_segment_ack_t*);
void SenderUpdateBase(uint32_t);
void RemoveAckedPacket(uint32_t seq);
void RegisterSenderInterface(SenderInterface*s){ mpsender_=s;}
void RegisterReceiverInterface(ReceiverInterface *r){mpreceiver_=r;}
void UpdateRtt(uint32_t time);
void UpdateMinRtt(uint32_t rtt);
void SenderStop();
void ReceiverStop();
void SendSegmentAck(sim_segment_ack_t *ack);
void SendFeedback(const uint8_t* payload, int payload_size);
void PaceSend(uint32_t packet_id, int retrans, size_t size, int padding);
private:
bool LossTableSeqExist(uint32_t);
void LossTableRemove(uint32_t);
void LossTableRemoveUntil(uint32_t);
void UpdataLoss(uint32_t);
void UpdataSendTs(uint32_t ts);
send_buf_t *AllocateSentBuf();
void FreePendingBuf();
void FreeSentBuf();
void RemoveSentBufUntil(uint32_t);
void SentBufCollection(uint32_t now);
public:
su_addr src;
su_addr dst;
su_socket fd;
uint8_t pid;
int state;
uint8_t con_c;
uint32_t con_ts;
uint32_t rtt_;
uint32_t min_rtt_;
uint32_t rtt_var_;
uint32_t rtt_update_ts_;
uint32_t ping_resend_;
uint16_t trans_seq_;
uint32_t packet_seed_;
uint32_t rate_;
uint32_t water_above_ts_;//path full use, may congestipon
uint32_t water_below_ts_;//path under use,
//for receiver
uint32_t base_seq_;
uint32_t s_sent_ts_; //smooth sent time at the sender side
uint32_t max_seq_;
uint32_t ack_ts_;
private:
void UpdataRecvTable(uint32_t seq);
void RecvTableRemoveUntil(uint32_t seq);
bool CheckRecvTableExist(uint32_t seq);
uint32_t GetMinRecvSeq();
uint32_t GetLossTableSize();
std::map<uint32_t,sim_segment_t*> buf_;
std::list<send_buf_t*> sent_buf_;
std::set<uint32_t> loss_;
std::set<uint32_t> recv_table_;
uint32_t len_;//byte
CongestionController *controller_;
uint32_t sent_buf_c_;
std::queue<send_buf_t*> free_buf_;
ReceiverInterface *mpreceiver_;
SenderInterface *mpsender_;
uint32_t receiver_last_heart_beat_;
uint32_t sender_last_heart_beat_;
bin_stream_t	strm_;
uint32_t last_sentbuf_collect_ts_;
};
}




#endif /* MULTIPATH_PATH_H_ */
