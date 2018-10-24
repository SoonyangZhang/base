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
}
#endif /* SIM_TRANSPORT_MPCOMMON_H_ */
