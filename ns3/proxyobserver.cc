#include "proxyobserver.h"
namespace ns3{
ProxyObserver::ProxyObserver(SenderInterface *sender,uint8_t pid){
	mpsender_=sender;
	pid_=pid;
}
void ProxyObserver::OnNetworkChanged(uint32_t bitrate_bps,
        uint8_t fraction_loss,  // 0 - 255.
        int64_t rtt_ms,
        int64_t probing_interval_ms){
	if(mpsender_){
		mpsender_->OnNetworkChanged(pid_,bitrate_bps,fraction_loss,rtt_ms);
	}
}
}



