#ifndef NS3_MPVIDEO_PROXYOBSERVER_H_
#define NS3_MPVIDEO_PROXYOBSERVER_H_
#include "mpcommon.h"
#include "modules/congestion_controller/include/send_side_congestion_controller.h"
namespace ns3{
class ProxyObserver:public webrtc::SendSideCongestionController::Observer{
public:
	ProxyObserver(SenderInterface *sender,uint8_t pid);
	~ProxyObserver(){}
	void OnNetworkChanged(uint32_t bitrate_bps,
	                      uint8_t fraction_loss,  // 0 - 255.
	                      int64_t rtt_ms,
	                      int64_t probing_interval_ms) override;
private:
	SenderInterface *mpsender_;
	uint8_t pid_;
};
}




#endif /* NS3_MPVIDEO_PROXYOBSERVER_H_ */
