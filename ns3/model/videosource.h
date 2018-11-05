#ifndef NS3_MPVIDEO_VIDEOSOURCE_H_
#define NS3_MPVIDEO_VIDEOSOURCE_H_
namespace ns3{
class VideoSource{
public:
	virtual ~VideoSource(){}
	virtual void Start()=0;
	virtual void Stop()=0;
};
}
#endif /* NS3_MPVIDEO_VIDEOSOURCE_H_ */
