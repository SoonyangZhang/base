#ifndef DATASINK_H_
#define DATASINK_H_
namespace zsy{
template<typename T>
class DataSink{
public:
	virtual void OnIncomingData(T*)=0;
	virtual ~DataSink(){}
};
}
#endif /* DATASINK_H_ */
