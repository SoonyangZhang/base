#ifndef MULTIPATH_PATHRECEIVER_H_
#define MULTIPATH_PATHRECEIVER_H_
#include<set>
#include "mpcommon.h"
#include "pathcommon.h"
#include "sim_proto.h"
#include "sessioninterface.h"
#include "cf_stream.h"

#include "rtc_base/timeutils.h"
#include "modules/congestion_controller/include/receive_side_congestion_controller.h"
#include "modules/rtp_rtcp/source/rtcp_packet/transport_feedback.h"
#include "modules/pacing/packet_router.h"
#include "system_wrappers/include/clock.h"
#include "modules/utility/include/process_thread.h"

namespace zsy{
class PathReceiver :public webrtc::PacketRouter{
public:
	PathReceiver(webrtc::ProcessThread *pm,webrtc::Clock *clock);
	~PathReceiver();
	void HeartBeat(uint32_t ts);
	bool TimeToSendPacket(uint32_t ssrc,
	                        uint16_t sequence_number,
	                        int64_t capture_timestamp,
	                        bool retransmission,
	                        const webrtc::PacedPacketInfo& packet_info) override;

	size_t TimeToSendPadding(size_t bytes,
	                           const webrtc::PacedPacketInfo& packet_info) override;

	uint16_t AllocateSequenceNumber() override;

	  // Called every time there is a new bitrate estimate for a receive channel
	  // group. This call will trigger a new RTCP REMB packet if the bitrate
	  // estimate has decreased or if no RTCP REMB packet has been sent for
	  // a certain time interval.
	  // Implements RtpReceiveBitrateUpdate.
	void SendConnectAck(uint32_t cid);
	void SendDisconnect(uint32_t now);
	void SendPingMsg(uint32_t now);
	void OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs,
	                               uint32_t bitrate_bps) override;
	bool SendTransportFeedback(webrtc::rtcp::TransportFeedback* packet) override;
	void UpdateRtt(uint32_t time,uint32_t now);
	void OnReceiveSegment(sim_header_t *header,sim_segment_t *seg);
	void VideoRealAck(int hb,uint32_t seq);
	void SendSegmentAck(sim_segment_ack_t *ack);
	void Consume(uint32_t packet_id);
	void RegisterReceiverInterface(ReceiverInterface *r){mpreceiver_=r;}
	void SendToNetwork(uint8_t*data,uint32_t len);
	void Stop();
	uint32_t GetWaittingDuration();
	uint32_t GetLossTableSize();
private:
	void ConfigureCongestion();
	bool LossTableSeqExist(uint32_t seq);
	void LossTableRemove(uint32_t seq);
	void LossTableRemoveUntil(uint32_t seq);
	void UpdataLoss(uint32_t seq);
	void UpdataRecvTable(uint32_t seq);
	void RecvTableRemoveUntil(uint32_t seq);
	bool CheckRecvTableExist(uint32_t seq);
	uint32_t GetMinRecvSeq();
public:
	su_addr src;
	su_addr dst;
	su_socket fd;
	uint8_t pid;
	int state;
	uint32_t rtt_;
	uint32_t sum_rtt_;
	uint32_t max_rtt_;
	uint32_t rtt_num_;
	uint32_t rtt_var_;
	uint32_t rtt_update_ts_;
	uint32_t ping_resend_;
private:
	uint32_t base_seq_;
	uint32_t s_sent_ts_; //smooth sent time at the sender side
	uint32_t max_seq_;
	uint32_t ack_ts_;
	bool first_packet_;
	bool stop_;
	std::set<uint32_t> loss_;
	std::set<uint32_t> recv_table_;
	CongestionController* controller_;
	ReceiverInterface *mpreceiver_;
	uint32_t receiver_last_heart_beat_;
	bin_stream_t	strm_;
	webrtc::ProcessThread *pm_;
	webrtc::Clock *clock_;
    webrtc::RemoteBitrateEstimator *rbe_;
};
}



#endif /* MULTIPATH_PATHRECEIVER_H_ */
