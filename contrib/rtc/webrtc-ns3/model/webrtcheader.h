#ifndef WEBRTC_HEADER_H
#define WEBRTC_HEADER_H
#include"ns3/type-id.h"
#include "ns3/header.h"
namespace ns3
{
enum ProtoType
{
	PT_MIN_MSG_ID	=0,

	PT_CONNECT,
	PT_CONNECT_ACK,		

	PT_DISCONNECT,
	PT_DISCONNECT_ACK,

	PT_PING,
	PT_PONG,

	PT_SEG,
	PT_SEG_ACK,
	PT_FEEDBACK,
	PT_MAX_MSG_ID		
};
typedef struct 
{
	uint8_t mid;
	uint32_t uid; //a user may have multiple data source audio video
}proto_header_t;
typedef struct
{
	uint32_t cid;//ssrc
}proto_connect_t;
typedef struct
{
	uint32_t cid;
	uint32_t result;
}proto_connect_ack_t;
typedef struct
{
	uint32_t cid;
}proto_disconnect_t;
typedef proto_disconnect_t proto_disconnect_ack_t;
typedef struct
{
	int64_t	ts;
}proto_ping_t;
typedef proto_ping_t proto_pong_t;
typedef struct
{
	uint32_t	packet_id;				/*包序号*/
	uint32_t	fid;					/*帧序号*/
	uint32_t	timestamp;				/*帧时间戳*/
	uint16_t	index;					/*帧分包序号*/
	uint16_t	total;					/*帧分包总数*/
	uint8_t		ftype;					/*视频帧类型*/

	uint8_t		remb;				
	uint16_t	send_ts;				/*发送时刻相对帧产生时刻的时间戳*/
	uint16_t	transport_seq;			/*传输通道序号，这个是传输通道每发送一个报文，它就自增长1，而且重发报文也会增长*/
	
	uint16_t	data_size;
}proto_segment_t;
#define WEBRTC_NACK_NUM	80
typedef struct
{
	uint32_t	base_packet_id;			/*被接收端确认连续最大的包ID*/
	uint32_t	acked_packet_id;		/*立即确认的报文序号id,用于计算rtt*/
	/*重传的序列*/
	uint8_t		nack_num;
	uint16_t	nack[WEBRTC_NACK_NUM];
}proto_segment_ack_t;
#define ProtoHeaderSize sizeof(ProtoHeader)
#define MAX_WEBRTC_HEADER_LEN 1400
class WebrtcHeader:public ns3::Header
{
public:
	WebrtcHeader();
	~WebrtcHeader();
	static TypeId GetTypeId();
	virtual ns3::TypeId GetInstanceTypeId (void) const;
	virtual uint32_t GetSerializedSize (void) const;
	void SetMid(uint8_t mid){m_mid=mid;}
	void SetUid(uint32_t uid){m_uid=uid;}
	uint8_t GetMid(){return m_mid;}
	uint32_t GetUid(){return m_uid;}
	uint32_t SetMessagePayload(void *buf,uint32_t buflen);
	uint32_t GetMessage(void *buf,uint32_t buflen);
	static uint32_t GetMessageLen(uint8_t mid,void *message);
    virtual void Serialize (ns3::Buffer::Iterator start) const;
    virtual uint32_t Deserialize (ns3::Buffer::Iterator start);
    virtual void Print (std::ostream &os) const;
private:
	uint8_t m_mid;
	uint32_t m_uid;
	uint8_t m_message[MAX_WEBRTC_HEADER_LEN];//message payload
	uint32_t m_len;//message legth
};
}
#endif
