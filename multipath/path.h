#ifndef MULTIPATH_PATH_H_
#define MULTIPATH_PATH_H_
#include "mpcommon.h"
#include "sessioninterface.h"
#include <map>
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
public:
su_addr src;
su_addr dst;
su_socket fd;
uint8_t pid;
int state;
uint8_t con_c;
uint32_t con_ts;
uint32_t rtt;
uint32_t rtt_update_ts;
uint16_t trans_seq;
uint32_t packet_seed_;
uint32_t rate;
uint32_t water_above_ts;//path full use, may congestipon
uint32_t water_below_ts;//path under use,
//for receiver
uint32_t base_seq_;
private:
std::map<uint32_t,sim_segment_t*> buf_;
std::map<uint32_t,sim_segment_t*> sent_buf_;
uint32_t len_;//byte
CongestionController *controller_;
};
}




#endif /* MULTIPATH_PATH_H_ */
