#ifndef SIM_TRANSPORT_MPRECEIVER_H_
#define SIM_TRANSPORT_MPRECEIVER_H_
#include <map>
#include "sessioninterface.h"
#include "path.h"
namespace zsy{
class MultipathReceiver{
public:
	MultipathReceiver(SessionInterface*session,uint32_t uid);
	~MultipathReceiver();
	void Drive();
	void ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream);
private:
	void SendConnectAck(PathInfo &path,uint32_t cid);
	uint32_t uid_;
	SessionInterface *session_;
	std::map<uint8_t,PathInfo> paths_;
	bin_stream_t	strm_;
};
}
#endif /* SIM_TRANSPORT_MPRECEIVER_H_ */
