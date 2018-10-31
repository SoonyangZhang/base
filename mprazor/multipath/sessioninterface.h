#ifndef SIM_TRANSPORT_SESSIONINTERFACE_H_
#define SIM_TRANSPORT_SESSIONINTERFACE_H_
#include "mpcommon.h"
namespace zsy{
enum ROLE{
	ROLE_SENDER,
	ROLE_RECEIVER,
};
class CongestionController{
public:
	CongestionController(void *session,uint8_t pid)
:session_(session)
,pid_(pid)
,s_cc_(NULL)
,r_cc_(NULL){
	}
	~CongestionController(){
		if(s_cc_){
			razor_sender_destroy(s_cc_);
		}
		if(r_cc_){
			razor_receiver_destroy(r_cc_);
		}
	}
	void SetCongestion(void *congestion,uint8_t role){
		if(role==ROLE_SENDER){
			s_cc_=(razor_sender_t*)congestion;
		}else{
			r_cc_=(razor_receiver_t*)congestion;
		}
	}
	void *session_;
	uint8_t pid_;
	razor_sender_t *s_cc_;
	razor_receiver_t *r_cc_;
};
}




#endif /* SIM_TRANSPORT_SESSIONINTERFACE_H_ */
