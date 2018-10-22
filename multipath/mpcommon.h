#ifndef SIM_TRANSPORT_MPCOMMON_H_
#define SIM_TRANSPORT_MPCOMMON_H_
#include "cf_platform.h"
#include "sim_internal.h"
namespace zsy{
enum PathState{
	path_ini,
	path_conning,
	path_conned,
	path_discon,
};
struct PathInfo{
	PathInfo()
	:fd(0),pid{0}
    ,state(path_ini)
	,con_c{0}{}
	su_addr src;
	su_addr dst;
	su_socket fd;
	uint8_t pid;
	int state;
	uint8_t con_c;
	uint32_t ts;
};
}




#endif /* SIM_TRANSPORT_MPCOMMON_H_ */
