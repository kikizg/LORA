#pragma once

#include "rn2483.h"

namespace RN
{
	// List of possible request from receiving node.
	enum ResponseStatus : unsigned char
	{
		RSOK,		// Response info is ok.
		RSWRONG,	// Response info is not valid (wrong parameters in response packet).
		RSRESEND	// Response info requires to resend data.
	};

	// Information for TX packet.
	struct PacketInfo
	{
		unsigned char FromId;		// Send from node (id).
		unsigned char ToId;		// Send to node (id). Use 0xffffffff to broadcast.
		unsigned char Port;		// Port of packet.
	};

	struct PacketInfoPart : public PacketInfo
	{
		unsigned char Size;		// Packet data size.
		unsigned char SegId;		// Packet segment id.
	};

	struct PacketInfoInit : public PacketInfoPart
	{
		bool Ack;			// Should receiving node acknowledge.
		size_t SizeTotal;		// Total size of data (in all packets).
	};

	// Acknowledge information on TX from receiving node.
	struct PacketInfoRsp : public PacketInfo
	{
		bool RequestResend;		// Packet is not received and request to resend it.
		unsigned char SegId;		// Packet segment id.
	};

	// Class used for exchanging data through rn2483 device.
	class Comm
	{
		public:
			// Default class constructor.
			Comm();

			// Class destructor.
			// Free  all resources.
			~Comm();

			// Initialize communication.
			// Returns true on success, false on failure.
			bool Init();

			// Set info for sending packet.
			// pInfo: Pointer to structure with packet information.
			// Returns true on success, false on failure.
			bool SetInfo(const PacketInfo *pInfo);

			// Send data through RN2483 device to specific node without receive acknowledge.
			// pData: Pointer to data which will be send.
			// szData: Size of data which will be send.
			// Returns true on success, false on failure.
			bool Send(const void *pData, size_t szData);

			// Send data through RN2483 device to specific node with receive acknowledge.
			// pData: Pointer to data which will be send.
			// szData: Size of data which will be send.
			// Returns true on success, false on failure.
			bool SendAck(const void *pData, size_t szData);

			// Receive data through RN2483 device from specific node.
			// pData: Pointer where received data will be stored.
			// szData: Size of buffer pData [byte].
			// Returns true on success, false on failure.
			bool Receive(void *pData, size_t szData);

		private:
			// Check if response packet for initial TX is ok.
			// Returns response status.
			ResponseStatus _okResInit();

			// Check if response packet for partial TX is ok.
			// Returns response status.
			ResponseStatus _okResPart();

			// Check if initial RX info packet is ok.
			ResponseStatus _okRXInit();

			// Check if partial RX info packet is ok.
			ResponseStatus _okRXPart();

			RN2483 _rn;			// Communication with RN2483 device.

			PacketInfoInit _TXInit;		// Init packet information structure on TX.
			PacketInfoPart _TXPart;		// Partial packet information structure on TX.

			static const size_t _szBufTX;	// Size of TX buffer [byte].
			unsigned char _szDataMaxInit;	// Max size of data in initial TX packet [byte].
			unsigned char _szDataMaxPart;	// Max size of data in partial TX packet [byte].

			static const size_t _szBufRX;	// Size of RX buffer [byte].
			char *_pRXBuf;			// Internal RX buffer.
			PacketInfoInit _RXInit;		// Init packet information structure on RX.
			PacketInfoRsp _RXRsp;		// Packet response which is send after successful RX (from receiving node).
			PacketInfoRsp *_pRXRsp;		// Packet response information on TX (from receiving node).
			PacketInfoInit *_pRXInit;	// Init packet information structure on RX.
			PacketInfoPart *_pRXPart;	// Partial packet information structure on RX.
			char *_pRXData;			// Internal RX data buffer.

			bool _bPckInfoSet;		// Is packet info for TX set.
	};

};
