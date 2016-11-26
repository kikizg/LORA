#pragma once

#include <vector>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#include "clock.h"
#include "rn2483.h"
// #include "uart.h"

namespace RN
{
	// Information for TX packet.
	struct PacketInfo
	{
		unsigned char LocalId;		// Id of local (this) node.
		unsigned char RemoteId;		// Id of remote node to which data will be send.
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
		size_t SizeTotal;		// Total size of data (in all packets) [byte].
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

			// Set encryption/decryption method. Method reads public and private keys and prepare
			// for encrypt/decrypt operations.
			// pPublic: Pointer to public key file name.
			// pPrivate: Pointer to private key file name.
			// Returns true on success, false on failure.
			bool SetCrypt(const char *pPublic, const char *pPrivate);

			// Send data through RN2483 device to specific node without receive acknowledge.
			// pData: Pointer to data which will be send.
			// szData: Size of data which will be send.
			// ack: Require successfull acknowledge after each TX from receiving node.
			// Returns true on success, false on failure.
			bool Send(const void *pData, size_t szData, bool ack = true);

			// Encrypt with public key and send data through RN2483 device to specific node without receive acknowledge.
			// pData: Pointer to data which will be send.
			// szData: Size of data which will be send.
			// ack: Require successfull acknowledge after each TX from receiving node.
			// Returns true on success, false on failure.
			bool EncryptPubSend(const void *pData, size_t szData, bool ack = true);

			// Encrypt with private key and send data through RN2483 device to specific node without receive acknowledge.
			// pData: Pointer to data which will be send.
			// szData: Size of data which will be send.
			// ack: Require successfull acknowledge after each TX from receiving node.
			// Returns true on success, false on failure.
			bool EncryptPvtSend(const void *pData, size_t szData, bool ack = true);

			// Receive data through RN2483 device from specific node.
			// pData: Pointer where received data will be stored.
			// szData: Size of buffer pData [byte].
			// szDataRX: Pointer to received data size [byte].
			// Returns true on success, false on failure.
			bool Receive(void *pData, size_t szData, size_t *pSzDataRX = NULL);

			// Receive data through RN2483 device from specific node and decrypt it with public key.
			// pData: Pointer where received data will be stored.
			// szData: Size of buffer pData [byte].
			// szDataRX: Pointer to received data size [byte].
			// Returns true on success, false on failure.
			bool ReceiveDecryptPub(void *pData, size_t szData, size_t *pSzDataRX = NULL);

			// Receive data through RN2483 device from specific node and decrypt it with private key.
			// pData: Pointer where received data will be stored.
			// szData: Size of buffer pData [byte].
			// szDataRX: Pointer to received data size [byte].
			// Returns true on success, false on failure.
			bool ReceiveDecryptPvt(void *pData, size_t szData, size_t *pSzDataRX = NULL);

			// Size of RX buffer [byte].
			size_t GetMaxSz();

			// Buffer for data encryption (initialized in SetCrypt method).
			size_t GetSzEncryptBuf();

			// Buffer for data decryption (initialized in SetCrypt method).
			size_t GetSzDecryptBuf();

		private:
			// Send initial or partial data and wait for acknowledge packet if requested.
			// pInfo: Pointer to packet info. If initial send is performed pInfo is
			// pointing  to PacketInfoInit. If partial send is performed pInfo is
			// pointing to PacketInfoPart.
			// szInfo: Size of packet info at address pInfo.
			// pData: Pointer to data which need to be send.
			bool _send(const PacketInfoPart *pInfo, size_t szInfo, const char *pData);
			bool _sendAck();

			bool _receive(char *pData, size_t szData);

			// Compare LocalId, RemoteId and Port of two packet info.
			// pInfoA: Pointer to first packet info.
			// pInfoB: Pointer to second packet info.
			// Returns true if packet infos are matched or false otherwise.
			bool _checkInfo(const PacketInfo *pInfoA, const PacketInfo *pInfoB);

			// Release existing crypt resources.
			void _releaseCrypt();

			Clock _clk;			// Time measuring mechanism for timeout.

			// RN2483 _rn;			// Communication with RN2483 device.
			RN2483 _rn;			// Communication with RN2483 device.
			static const char _retrySend;	// Number of attempts to send packet before error is raised.
			static const char _retryTX;	// Number of attempts to send data before error is raised.
			static const char _retryTXAck;	// Number of attempts to send ack packet before error is raised.

			PacketInfoInit _TXInit;		// Init packet information structure on TX (for internal use).
			PacketInfoPart _TXPart;		// Partial packet information structure on TX (for internal use).

			static const size_t _szBufTX;	// Size of TX buffer [byte].
			unsigned char _szDataMaxInit;	// Max size of data in initial TX packet [byte].
			unsigned char _szDataMaxPart;	// Max size of data in partial TX packet [byte].
			size_t _szDataMax;		// Max size of data to send regardless packet info segment limitation (PacketInfoPart::SegId is unsigned char and it cannot be greater than 255 individual messages) [byte].


			static const double _toRecv;	// Timeout for receiving data [second].
			static const double _toAck;	// Timeout for receiving ack packet. Afterwards data packet will be resend [second].

			PacketInfoInit _RXInfo;		// Init packet information structure on RX (for internal use).
			PacketInfoRsp _RXRsp;		// Packet response which is send after successful RX (from receiving node).
			static const size_t _szBufRX;	// Size of RX buffer [byte].
			char *_pRXBuf;			// Internal RX buffer.
			PacketInfoRsp *_pRXRsp;		// Packet response information on TX (from receiving node).
			PacketInfoInit *_pRXInit;	// Init packet information structure on RX.
			PacketInfoPart *_pRXPart;	// Partial packet information structure on RX.

			bool _bPckInfoSet;		// Is packet info for TX set.

			BIO *_pPublic;			// Public key for crypt method.
			BIO *_pPrivate;			// Private key for crypt method.
			RSA *_pRSAPvt;			// Private RSA class for crypt method.
			RSA *_pRSAPub;			// Public RSA class for crypt method.
			unsigned char *_pDecryptBuf;	// Buffer for data decryption (initialized in SetCrypt method).
			unsigned char *_pEncryptBuf;	// Buffer for data encryption (initialized in SetCrypt method).
			size_t _szDecryptBuf;		// Size of buffer for data decrpytion.
			size_t _szEncryptBuf;		// Size of buffer for data encryption.
			size_t _szRSAPub;		// Size of RSA modulus [byte].
			size_t _szRSAPvt;		// Maximum size of data to encrypt in one pass [byte].
			char *_pChk;
	};

};
