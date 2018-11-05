#ifndef NS3_MPVIDEO_MPVIDEOHEADER_H_
#define NS3_MPVIDEO_MPVIDEOHEADER_H_
#include"ns3/type-id.h"
#include "ns3/header.h"

namespace ns3{
enum PayloadType{
	razor,
	webrtc_fb,
};
class MpvideoHeader:public ns3::Header{
public:
	MpvideoHeader(){}
	~MpvideoHeader(){}
	static TypeId GetTypeId();
	virtual ns3::TypeId GetInstanceTypeId (void) const;
	virtual uint32_t GetSerializedSize (void) const;
	void SetMid(uint8_t pt){pt_=pt;}
    uint8_t GetPayloadType(){return pt_;}
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
private:
	uint8_t pt_;
};
}




#endif /* NS3_MPVIDEO_MPVIDEOHEADER_H_ */
