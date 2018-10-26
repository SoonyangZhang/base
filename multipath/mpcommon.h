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
class NetworkDataCallback{
public:
	virtual ~NetworkDataCallback(){}
	virtual void ForwardUp(uint32_t fid,uint8_t*data,uint32_t len,bool full)=0;
};
}
#endif /* SIM_TRANSPORT_MPCOMMON_H_ */
