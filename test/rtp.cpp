#include "test.h"
#include "rtp.h"

class RTPTestPlan: public TestPlan
{
public:
	RTPTestPlan() : TestPlan("RTP test plan")
	{
		
	}
	
	int init() 
	{
		Log("RTP::Init\n");
		return true;
	}
	
	int end()
	{
		Log("RTP::End\n");
		return true;
	}
	
	
	virtual void Execute()
	{
		init();
		
		Log("RTPHeader\n");
		testRTPHeader();
		Log("testSenderReport\n");
		testSenderReport();
		Log("NACK\n");
		testNack();
		Log("SDES header extensions\n");
		testSDESHeaderExtensions();
		Log("Transport Wide Message Feedback (1)\n");
		testTransportField();
		Log("Transport Wide Message Feedback (2)\n");
		testTransportWideFeedbackMessage();
		end();
	}
	
	int testRTPHeader()
	{
		RTPHeader header;
		RTPHeaderExtension extension;
		RTPMap	extMap;
		extMap[5] = RTPHeaderExtension::TransportWideCC;
		
		BYTE data[]= {	
				0x90,0xe2,0x7c,0x2d,0x96,0x92,0x68,0x3a,0x00,0x00,0x62,0x32, //RTP Header
				0xbe,0xde,0x00,0x01,0x51,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		DWORD size = sizeof(data);
		
		//Parse header
		DWORD len = header.Parse(data,size);
		
		//Dump
		header.Dump();
		
		//Check data
		assert(len);
		assert(header.version==2);
		assert(!header.padding);
		assert(header.extension);
		assert(header.payloadType==98);
		assert(header.csrcs.size()==0);
		assert(header.ssrc==0x6232);
		assert(header.mark);
		assert(header.timestamp=252617738);
		
		//Parse extensions
		len = extension.Parse(extMap,data+len,size-len);
		
		//Dump
		header.Dump();
		extension.Dump();
		
		//Check
		assert(len);
		assert(extension.hasTransportWideCC);
		
		//Copy header
		RTPHeader clone(header);
		//Check data
		assert(clone.version==2);
		assert(!clone.padding);
		assert(clone.extension);
		assert(clone.payloadType==98);
		assert(clone.csrcs.size()==0);
		assert(clone.ssrc==0x6232);
		assert(clone.mark);
		assert(clone.timestamp=252617738);
		clone.Dump();
	}
	

	void testSenderReport()
	{
		BYTE msg[] = {
			0x80,  0xc8,  0x00,  0x06,
			
			0x95,  0xce,  0x79,  0x1d,  //<-Sender
			0xdc,  0x48,  0x24,  0x68,  //NTP ts
			0xf7,  0x8e,  0xb0,  0x32, 
			0xe3,  0x4b,  0xae,  0x04,  //RTP ts
			0x00,  0x00,  0x00,  0xec,  //sende pc
			0x00,  0x01,  0x97,  0x22,  //sender oc
			
			0x81,  0xca,  0x00,  0x06,  //SSRC 1
			0x95,  0xce,  0x79,  0x1d,  //
			0x01,  0x10,  0x43,  0x4a,  //
			0x54,  0x62,  0x63,  0x47,  // 
			0x48,  0x36,  0x50,  0x59,  // 
			0x44,  0x4c,  0x43,  0x6c,  // 
			0x69,  0x34,  0x00,  0x00,  //
		};
		
		//Parse it
		RTCPCompoundPacket* rtcp = RTCPCompoundPacket::Parse(msg,sizeof(msg));
		rtcp->Dump();
		
		//Serialize
		BYTE data[1024];
		DWORD len = rtcp->Serialize(data,1024);
		Dump(data,len);
		
		assert(RTCPCompoundPacket::IsRTCP(data,len));
		assert(len==sizeof(msg));
		assert(memcmp(msg,data,len)==0);
		
		//Dump it
		rtcp->Dump();
		
		delete(rtcp);

	}
	
	void testNack()
	{
		
		BYTE msg[] = {
				0x81,   0xcd,   0x00,   0x03,  0x8a,   0xd7,   0xb4,   0x92, 
				0x00,  0x00,   0x61,   0x3b,   0x10,   0x30,   0x3,   0x00 
		};
			/*
			[RTCPCompoundPacket count=1 size=20]
			[RTCPPacket Feedback NACK sender:2329392274 media:24891]
				   [NACK pid:4144 blp:0000001100000000 /]
			[/RTCPPacket Feedback NACK]
			[/RTCPCompoundPacket]
			 */
		BYTE data[1024];
		
		//Create rtcp sender retpor
		RTCPCompoundPacket rtcp;

		//Create NACK
		RTCPRTPFeedback *nack = RTCPRTPFeedback::Create(RTCPRTPFeedback::NACK,2329392274,24891);

		//Add 
		nack->AddField(new RTCPRTPFeedback::NACKField(4144,(WORD)0x0300));

		//Add to packet
		rtcp.AddRTCPacket(nack);
		
		rtcp.Dump();
		
		//Serialize it
		DWORD len = rtcp.Serialize(data,1024);
		
		Dump(data,len);
		
		assert(RTCPCompoundPacket::IsRTCP(data,len));
		assert(len==sizeof(msg));
		assert(memcmp(msg,data,len)==0);
		
		//parse it back
		RTCPCompoundPacket* cloned = RTCPCompoundPacket::Parse(data,len);
		
		assert(cloned);
		
		//Serialize it again
		len = rtcp.Serialize(data,1024);
		
		Dump(data,len);
		
		assert(RTCPCompoundPacket::IsRTCP(data,len));
		assert(len==sizeof(msg));
		assert(memcmp(msg,data,len)==0);
		
		//Dump it
		cloned->Dump();
		
		delete(cloned);
	}
	
	void testTransportField()
	{
		BYTE aux[1024];
		BYTE data[] = { 
		0x8f,0xcd,0x00,0x05,
		0x29,0xdd,0x06,0xa4,
		0x00,0x00,0x27,0x50,
		
		0x04,0x1e,0x00,0x02,
		0x09,0x03,0x95,0xed,
		0xc4,0x00,0x0c,0x00 };

		DWORD size = sizeof(data);
		
		//Parse it
		RTCPCompoundPacket* rtcp = RTCPCompoundPacket::Parse(data,size);
		rtcp->Dump();
		
		//Serialize
		DWORD len = rtcp->Serialize(aux,1024);
		Dump(data,sizeof(data));
		Dump(aux,len);
		
		RTCPCompoundPacket* cloned = RTCPCompoundPacket::Parse(aux,len);
		cloned->Dump();
		
		delete(rtcp);
		delete(cloned);
		
		BYTE data2[] = {
		0x8f,0xcd,0x00,0x05,
		0x97,0xb3,0x78,0x4f,
		0x00,0x00,0x40,0x4f,
		
		0x00,0x45,0x00,0x01,
		0x09,0xf9,0x73,0x00,
		0x20,0x01,0x0c,0x00};

		DWORD size2 = sizeof(data2);
		
		//Parse it
		RTCPCompoundPacket* rtcp2 = RTCPCompoundPacket::Parse(data2,size2);
		rtcp2->Dump();
		

		len = rtcp2->Serialize(aux,1024);
		Dump(data2,sizeof(data));
		Dump(aux,len);
		RTCPCompoundPacket* cloned2 = RTCPCompoundPacket::Parse(aux,len);
		cloned2->Dump();
		
		delete(rtcp2);
		delete(cloned2);
		
	}
	
	void testSDESHeaderExtensions()
	{
		constexpr int8_t kPayloadType = 100;
		constexpr uint16_t kSeqNum = 0x1234;
		constexpr uint8_t kSeqNumFirstByte = kSeqNum >> 8;
		constexpr uint8_t kSeqNumSecondByte = kSeqNum & 0xff;
		constexpr uint8_t RTPStreamId = 0x0a;
		constexpr uint8_t MediaStreamId = 0x0b;
		constexpr uint8_t RepairedRTPStreamId = 0x0c;
		constexpr char kStreamId[] = "streamid";
		constexpr char kRepairedStreamId[] = "repairedid";
		constexpr char kMid[] = "mid";
		constexpr uint8_t kPacketWithRsid[] = {
		    0x90, kPayloadType, kSeqNumFirstByte, kSeqNumSecondByte,
		    0x65, 0x43, 0x12, 0x78,
		    0x12, 0x34, 0x56, 0x78,
		    0xbe, 0xde, 0x00, 0x03,
		    0xa7, 's',  't',  'r',
		    'e',  'a',  'm',  'i',
		    'd' , 0x00, 0x00, 0x00};

		constexpr uint8_t kPacketWithMid[] = {
		    0x90, kPayloadType, kSeqNumFirstByte, kSeqNumSecondByte,
		    0x65, 0x43, 0x12, 0x78,
		    0x12, 0x34, 0x56, 0x78,
		    0xbe, 0xde, 0x00, 0x01,
		    0xb2, 'm',  'i',  'd'};
		
		constexpr uint8_t kPacketWithAll[] = {
		    0x90, kPayloadType, kSeqNumFirstByte, kSeqNumSecondByte,
		    0x65, 0x43, 0x12, 0x78,
		    0x12, 0x34, 0x56, 0x78,
		    0xbe, 0xde, 0x00, 0x06,
		    0xb2, 'm',  'i',  'd',
	            0xa7, 's',  't',  'r',
		    'e',  'a',  'm',  'i',
		    'd',  0xc9, 'r',  'e',
		    'p',  'a',  'i',  'r',
		    'e',  'd',  'i',  'd'};
		
		RTPMap	extMap;
		extMap[RTPStreamId] = RTPHeaderExtension::RTPStreamId;
		extMap[MediaStreamId] = RTPHeaderExtension::MediaStreamId;
		extMap[RepairedRTPStreamId] = RTPHeaderExtension::RepairedRTPStreamId;
		
		//RID
		{
			RTPHeader header;
			RTPHeaderExtension extension;
			//Parse header with rid
			const BYTE* data = kPacketWithRsid;
			DWORD size = sizeof(kPacketWithRsid);
			DWORD len = header.Parse(data,size);

			//Dump
			header.Dump();

			//Check data
			assert(len);
			assert(header.version==2);
			assert(!header.padding);
			assert(header.extension);
			assert(header.payloadType==kPayloadType);
			assert(header.sequenceNumber==kSeqNum);

			//Parse extensions
			len = extension.Parse(extMap,data+len,size-len);

			//Dump
			extension.Dump();

			//Check
			assert(len);
			assert(extension.hasRTPStreamId);
			assert(strcmp(extension.rid.c_str(),kStreamId)==0);
			
			//Serialize and ensure it is the same
			BYTE aux[128];
			len = header.Serialize(aux,128);
			len += extension.Serialize(extMap,aux+len,size-len);
			Dump4(data,size);
			Dump4(aux,len);
			assert(len==size);
		}
		
		//MID
		{
			RTPHeader header;
			RTPHeaderExtension extension;
			//Parse header witn mid
			const BYTE* data = kPacketWithMid;
			DWORD size = sizeof(kPacketWithMid);
			DWORD len = header.Parse(data,size);
			//Dump
			header.Dump();

			//Check data
			assert(len);
			assert(header.version==2);
			assert(!header.padding);
			assert(header.extension);
			assert(header.payloadType==kPayloadType);
			assert(header.sequenceNumber==kSeqNum);
		
			//Parse extensions
			len = extension.Parse(extMap,data+len,size-len);

			//Dump
			extension.Dump();

			//Check
			assert(len);
			assert(extension.hasMediaStreamId);
			assert(strcmp(extension.mid.c_str(),kMid)==0);
			
			//Serialize and ensure it is the same
			BYTE aux[128];
			len = header.Serialize(aux,128);
			len += extension.Serialize(extMap,aux+len,size-len);
			Dump4(data,size);
			Dump4(aux,len);
			assert(len==size);
		}
		
		//ALL
		{
			RTPHeader header;
			RTPHeaderExtension extension;
			//Parse header with all
			const BYTE* data = kPacketWithAll;
			DWORD size = sizeof(kPacketWithAll);
			DWORD len = header.Parse(data,size);

			//Dump
			header.Dump();

			//Check data
			assert(len);
			assert(header.version==2);
			assert(!header.padding);
			assert(header.extension);
			assert(header.payloadType==kPayloadType);
			assert(header.sequenceNumber==kSeqNum);

			//Parse extensions
			len = extension.Parse(extMap,data+len,size-len);

			//Dump
			extension.Dump();

			//Check
			assert(len);
			assert(extension.hasRTPStreamId);
			assert(strcmp(extension.rid.c_str(),kStreamId)==0);
			assert(extension.hasRepairedRTPStreamId);
			assert(strcmp(extension.repairedId.c_str(),kRepairedStreamId)==0);
			assert(extension.hasMediaStreamId);
			assert(strcmp(extension.mid.c_str(),kMid)==0);
			
			//Serialize and ensure it is the same
			BYTE aux[128];
			len = header.Serialize(aux,128);
			len += extension.Serialize(extMap,aux+len,size-len);
			Dump4(data,size);
			Dump4(aux,len);
			assert(len==size);
		}
		
		//MIX - no serialize transport wide cc
		{
			RTPMap	map;
			map[1] = RTPHeaderExtension::AbsoluteSendTime;
			map[2] = RTPHeaderExtension::TimeOffset;
			map[3] = RTPHeaderExtension::MediaStreamId;
			map[4] = RTPHeaderExtension::FrameMarking;
			
			RTPHeaderExtension extension;
			extension.hasTimeOffset = true;
			extension.hasAbsSentTime = true;
			extension.hasTransportWideCC = true;
			extension.hasFrameMarking = true;
			extension.hasMediaStreamId = true;
			extension.timeOffset = 1800;
			extension.absSentTime = 15069;
			extension.transportSeqNum = 116;
			extension.frameMarks.startOfFrame = 0;
			extension.frameMarks.endOfFrame = 1;
			extension.frameMarks.independent = 0;
			extension.frameMarks.discardable = 0;
			extension.frameMarks.baseLayerSync = 0;
			extension.frameMarks.temporalLayerId = 0;
			extension.frameMarks.spatialLayerId = 0;
			extension.frameMarks.tl0PicIdx = 0;
			extension.mid = "sdparta_0";
			
			
			//Serialize and ensure it is the same
			BYTE aux[128];
			extension.Dump();
			size_t len = extension.Serialize(map,aux,128);
			Dump4(aux,len);
			assert(0);

		}
	}
	
	void testTransportWideFeedbackMessage()
	{
		DWORD mainSSRC = 1;
		DWORD ssrc = 2;
		DWORD feedbackPacketCount = 1;
		std::map<DWORD,QWORD> packets;
		DWORD lastFeedbackPacketExtSeqNum = 0;
		DWORD feedbackCycles = 0;
		
		DWORD i=1;
		packets[i++] = 1000;
		packets[i++] = 2000;
		i++; //lost;
		packets[i++] = 3000;
		packets[i++] = 5000;
		packets[i++] = 4000; //OOO
		packets[i++] = 6000;
		i++; //lost;
		packets[i++] = 7000;
		
		//Create rtcp transport wide feedback
		RTCPCompoundPacket rtcp;

		//Add to rtcp
		RTCPRTPFeedback* feedback = RTCPRTPFeedback::Create(RTCPRTPFeedback::TransportWideFeedbackMessage,mainSSRC,ssrc);

		//Create trnasport field
		RTCPRTPFeedback::TransportWideFeedbackMessageField *field = new RTCPRTPFeedback::TransportWideFeedbackMessageField(feedbackPacketCount++);

		//For each packet stats
		for (auto it=packets.begin();it!=packets.end();++it)
		{
			//Get transportSeqNum
			DWORD transportSeqNum = it->first;
			//Get time
			QWORD time = it->second;

			//Check if we have a sequence wrap
			if (transportSeqNum<0x0FFF && (lastFeedbackPacketExtSeqNum & 0xFFFF)>0xF000)
				//Increase cycles
				feedbackCycles++;

			//Get extended value
			DWORD transportExtSeqNum = feedbackCycles<<16 | transportSeqNum;

			//if not first and not out of order
			if (lastFeedbackPacketExtSeqNum && transportExtSeqNum>lastFeedbackPacketExtSeqNum)
				//For each lost
				for (DWORD i = lastFeedbackPacketExtSeqNum+1; i<transportExtSeqNum; ++i)
					//Add it
					field->packets.insert(std::make_pair(i,0));
			//Store last
			lastFeedbackPacketExtSeqNum = transportExtSeqNum;

			//Add this one
			field->packets.insert(std::make_pair(transportSeqNum,time));

			//Delete from map
			packets.erase(it);
		}

		//And add it
		feedback->AddField(field);
			
		//Add it
		rtcp.AddRTCPacket(feedback);

		rtcp.Dump();
		
		//Get sizze
		DWORD size = rtcp.GetSize();
		
		//Create aux buffer for serializing it
		BYTE* data = (BYTE*)malloc(size);
		
		//Seriaize
		DWORD len = rtcp.Serialize(data,size);
		
		assert(len==size);
		Dump4(data,len);
		
		//Parse it
		RTCPCompoundPacket* parsed = RTCPCompoundPacket::Parse(data,len);
		
		parsed->Dump();
		
		free(data);
		delete(parsed);
	}
	
};

RTPTestPlan rtp;
