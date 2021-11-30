#include "webrtcheader.h"
#include<string.h>
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("WebrtcHeader");
NS_OBJECT_ENSURE_REGISTERED (WebrtcHeader);
WebrtcHeader::WebrtcHeader()
{
	memset(m_message,0,MAX_WEBRTC_HEADER_LEN);
}
WebrtcHeader::~WebrtcHeader(){}
TypeId WebrtcHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("WebrtcHeader")
      .SetParent<Header> ()
      .AddConstructor<WebrtcHeader> ()
    ;
    return tid;	
}
ns3::TypeId WebrtcHeader::GetInstanceTypeId() const
{
	return GetTypeId();
}
uint32_t WebrtcHeader::GetSerializedSize() const
{
	uint32_t hdlen=(sizeof(m_mid)+sizeof(m_uid));
	uint32_t msglen=0;
	switch (m_mid)
	{
		case PT_CONNECT:
		{
			msglen=sizeof(proto_connect_t);
		}
		break;
		case PT_CONNECT_ACK:
		{
			msglen=sizeof(proto_connect_ack_t);
		}
		break;
		case PT_DISCONNECT:
		case PT_DISCONNECT_ACK:
		{
			msglen=sizeof(proto_disconnect_t);
		}
		break;
		case PT_PING:
		case PT_PONG:
		{
			msglen=sizeof(proto_ping_t);
		}
		break;
		case PT_SEG:
		{
			msglen=sizeof(proto_segment_t);
		}
		break;
		case PT_SEG_ACK:
		{
			msglen=sizeof(proto_segment_ack_t);
		}
		break;
		case PT_FEEDBACK:
		{
			msglen=0;//message removed to the payload;
		}
		break;
		default:
		msglen=0;
		break;
	}
	return hdlen+msglen;
}
uint32_t WebrtcHeader::SetMessagePayload(void *buf,uint32_t buflen)
{
	NS_ASSERT_MSG(0<buflen&&buflen<=MAX_WEBRTC_HEADER_LEN,"out of lenfth");
	uint32_t len=GetMessageLen(m_mid,buf);
	uint8_t *byte=(uint8_t*)buf;	
	memcpy(m_message,byte,len);
	return len;
}
uint32_t WebrtcHeader::GetMessage(void *buf,uint32_t buflen)
{
	uint32_t len=GetMessageLen(m_mid,(void*)m_message);
	if(len>buflen){return 0;}
	memcpy(buf,m_message,len);
	return len;
}
uint32_t WebrtcHeader::GetMessageLen(uint8_t mid,void *message)
{
	uint32_t len=0;
	switch (mid)
	{
		case PT_CONNECT:
		{
			len=sizeof(proto_connect_t);
		}
		break;
		case PT_CONNECT_ACK:
		{
			len=sizeof(proto_connect_ack_t);
		}
		break;
		case PT_DISCONNECT:
		case PT_DISCONNECT_ACK:
		{
			len=sizeof(proto_disconnect_t);
		}
		break;
		case PT_PING:
		case PT_PONG:
		{
			len=sizeof(proto_ping_t);
		}
		break;
		case PT_SEG:
		{
			len=sizeof(proto_segment_t);
		}
		break;
		case PT_SEG_ACK:
		{
			proto_segment_ack_t *seg_ack=(proto_segment_ack_t*)message;
			len=sizeof(seg_ack->acked_packet_id)+sizeof(seg_ack->base_packet_id)+
					sizeof(seg_ack->nack_num)+seg_ack->nack_num*sizeof(uint16_t);			
		}
		break;
		case PT_FEEDBACK:
		{
			len=0;//message removed to the payload;
		}
		break;
		default:
		break;		
	}
	return len;	
}
void WebrtcHeader::Serialize (Buffer::Iterator start) const
{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;
	i.WriteU8(m_mid);
	i.WriteHtonU32(m_uid);
	NS_ASSERT(ProtoType::PT_MIN_MSG_ID<m_mid&&m_mid<ProtoType::PT_MAX_MSG_ID);
	switch (m_mid)
	{
		case PT_CONNECT:
		{
			proto_connect_t *con=(proto_connect_t*)m_message;
			i.WriteHtonU32(con->cid);
		}
		break;
		case PT_CONNECT_ACK:
		{
			proto_connect_ack_t *ack=(proto_connect_ack_t*)m_message;
			i.WriteHtonU32(ack->cid);
			i.WriteHtonU32(ack->result);
		}
		break;
		case PT_DISCONNECT:
		{
			proto_disconnect_t *discon=(proto_disconnect_t*)m_message;
			i.WriteHtonU32(discon->cid);
		}
		break;
		case PT_DISCONNECT_ACK:
		{
			proto_disconnect_ack_t *discon_ack=(proto_disconnect_ack_t*)m_message;
			i.WriteHtonU32(discon_ack->cid);
		}
		break;
		case PT_PING:
		{
			proto_ping_t *ping=(proto_ping_t*)m_message;
			i.WriteHtonU64((uint64_t)ping->ts);
		}
		break;
		case PT_PONG:
		{
			proto_pong_t *pong=(proto_pong_t*)m_message;
			i.WriteHtonU64((uint64_t)pong->ts);
		}
		break;
		case PT_SEG:
		{
			proto_segment_t *seg=(proto_segment_t*)m_message;
			i.WriteHtonU32(seg->packet_id);
			i.WriteHtonU32(seg->fid);
			i.WriteHtonU32(seg->timestamp);
			i.WriteHtonU16(seg->index);
			i.WriteHtonU16(seg->total);
			i.WriteU8(seg->ftype);
			i.WriteU8(seg->remb);
			i.WriteHtonU16(seg->send_ts);
			i.WriteHtonU16(seg->transport_seq);
			i.WriteHtonU16(seg->data_size);
		}
		break;
		case PT_SEG_ACK:
		{
			proto_segment_ack_t *seg_ack=(proto_segment_ack_t*)m_message;
			i.WriteHtonU32(seg_ack->base_packet_id);
			i.WriteHtonU32(seg_ack->acked_packet_id);
			uint8_t nack_num=seg_ack->nack_num;
			i.WriteU8(nack_num);
			uint8_t index=0;
			for(index=0;index<nack_num;index++)
			{
				i.WriteHtonU16(seg_ack->nack[index]);
			}
		}
		break;
		case PT_FEEDBACK:
		{
		NS_LOG_INFO("feedback message");
		}
		break;
		default:
		break;		
	}	
}
uint32_t WebrtcHeader::Deserialize (Buffer::Iterator start)
{
	NS_LOG_FUNCTION (this << &start);
    Buffer::Iterator i = start;	
	m_mid=i.ReadU8();
	m_uid=i.ReadNtohU32();	
	switch (m_mid)
	{
	case PT_CONNECT:
	{
		proto_connect_t *con=(proto_connect_t*)m_message;
		con->cid=i.ReadNtohU32();
	}
	break;
	case PT_CONNECT_ACK:
	{
		proto_connect_ack_t *ack=(proto_connect_ack_t*)m_message;
		ack->cid=i.ReadNtohU32();
		ack->result=i.ReadNtohU32();
	}
	break;
	case PT_DISCONNECT:
	{
		proto_disconnect_t *discon=(proto_disconnect_t*)m_message;
		discon->cid=i.ReadNtohU32();
	}
	break;
	case PT_DISCONNECT_ACK:
	{
		proto_disconnect_ack_t *discon_ack=(proto_disconnect_ack_t*)m_message;
		discon_ack->cid=i.ReadNtohU32();
	}
	break;
	case PT_PING:
	{
		proto_ping_t *ping=(proto_ping_t*)m_message;
		ping->ts=(int64_t)i.ReadNtohU64();
	}
	break;
	case PT_PONG:
	{
		proto_pong_t * pong=(proto_pong_t*)m_message;
		pong->ts=(int64_t)i.ReadNtohU64();
	}
	break;
	case PT_SEG:
	{
		proto_segment_t *seg=(proto_segment_t*)m_message;
		seg->packet_id=i.ReadNtohU32();
		seg->fid=i.ReadNtohU32();
		seg->timestamp=i.ReadNtohU32();
		seg->index=i.ReadNtohU16();
		seg->total=i.ReadNtohU16();
		seg->ftype=i.ReadU8();
		seg->remb=i.ReadU8();
		seg->send_ts=i.ReadNtohU16();
		seg->transport_seq=i.ReadNtohU16();
		seg->data_size=i.ReadNtohU16();
	}
	break;
	case PT_SEG_ACK:
	{
		proto_segment_ack_t *seg_ack=(proto_segment_ack_t*)m_message;
		seg_ack->base_packet_id=i.ReadNtohU32();
		seg_ack->acked_packet_id=i.ReadNtohU32();
		seg_ack->nack_num=i.ReadU8();
		uint8_t nack_num=seg_ack->nack_num;
		uint8_t index=0;
		for(index=0;index<nack_num;index++)
		{
			seg_ack->nack[index]=i.ReadNtohU16();
		}
	}
	break;
	case PT_FEEDBACK:
	{
		NS_LOG_ERROR("PT_FEEDBACK message should remove to the payload");
	}
	break;
	default:
	break;
	}
	return GetSerializedSize();
}
void WebrtcHeader::Print(std::ostream &os) const
{
    NS_LOG_FUNCTION (this << &os);
	switch(m_mid)
	{
		case PT_CONNECT:
		os<<"pt_connect";
		break;
		case PT_CONNECT_ACK:
		os<<"pt_connect_ack";
		break;
		case PT_DISCONNECT:
		os<<"pt_disconnect";
		break;
		case PT_DISCONNECT_ACK:
		os<<"pt_disconnect_ack";
		break;
		case PT_PING:
		os<<"pt_ping";
		break;
		case PT_PONG:
		os<<"pt_pong";
		break;
		case PT_SEG:
		os<<"pt_seg";
		break;
		case PT_SEG_ACK:
		os<<"pt_seg_ack";
		break;
		case PT_FEEDBACK:
		os<<"pt_feed_back";
		break;
		default:
		os<<"no this message type";
		break;
	}
	os<<"uid"<<m_uid;
}
}
