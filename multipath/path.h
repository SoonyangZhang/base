#ifndef MULTIPATH_PATH_H_
#define MULTIPATH_PATH_H_
#include "mpcommon.h"
#include "sessioninterface.h"
#include <map>
#include <list>
namespace zsy{
class PathInfo{
public:
PathInfo();
~PathInfo();
bool put(sim_segment_t*seg);
sim_segment_t *get_segment(uint32_t packet_id,int retrans);
uint32_t get_delay();
uint32_t get_len();
void SetController(CongestionController*);
CongestionController* GetController();
void OnReceiveSegment(sim_segment_t *seg);
void Consume(uint32_t packet_id);
void ReceiverUpdateLoss(uint32_t seq,uint32_t ts);
public:
su_addr src;
su_addr dst;
su_socket fd;
uint8_t pid;
int state;
uint8_t con_c;
uint32_t con_ts;
uint32_t rtt_;
uint32_t rtt_var_;
uint32_t rtt_update_ts_;
uint16_t trans_seq_;
uint32_t packet_seed_;
uint32_t rate_;
uint32_t water_above_ts_;//path full use, may congestipon
uint32_t water_below_ts_;//path under use,
//for receiver
uint32_t base_seq_;
uint32_t s_sent_ts_; //smooth sent time at the sender side
uint32_t max_seq_;
private:
std::map<uint32_t,sim_segment_t*> buf_;
std::map<uint32_t,sim_segment_t*> sent_buf_;
std::list<uint32_t> loss_;
uint32_t len_;//byte
CongestionController *controller_;
};
}




#endif /* MULTIPATH_PATH_H_ */
