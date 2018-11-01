#ifndef SIM_TRANSPORT_SESSIONINTERFACE_H_
#define SIM_TRANSPORT_SESSIONINTERFACE_H_
#include "mpcommon.h"
#include "modules/congestion_controller/include/send_side_congestion_controller.h"
#include "modules/congestion_controller/include/receive_side_congestion_controller.h"
namespace zsy{
enum ROLE{
	ROLE_SENDER,
	ROLE_RECEIVER,
};
class CongestionController{
public:
	CongestionController(void *congestion,uint8_t role)
:s_cc_(NULL)
,r_cc_(NULL){
		role_=role;
		if(role_==ROLE_SENDER){
			s_cc_=(webrtc::SendSideCongestionController*)congestion;
		}else{
			r_cc_=(webrtc::ReceiveSideCongestionController*)congestion;
		}
	}
	~CongestionController(){
		if(s_cc_){
			delete s_cc_;
		}
		if(r_cc_){
			delete r_cc_;
		}
	}
	uint8_t role_;
	webrtc::SendSideCongestionController *s_cc_;
	webrtc::ReceiveSideCongestionController *r_cc_;
};
}




#endif /* SIM_TRANSPORT_SESSIONINTERFACE_H_ */
