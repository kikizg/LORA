#include "comm.h"

using namespace RN;

#define WHITE "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BROWN "\033[1;33m"

#define _DEBUG

#ifdef _DEBUG
#define _DEBUG_CRYPT
#define _DEBUG_COMM_SR
#define _DEBUG_COMM_TR
#endif

const size_t Comm::_szBufTX = 63;
const size_t Comm::_szBufRX = 63;
const char Comm::_retrySend = 1;
const char Comm::_retryTX = 1;
const char Comm::_retryTXAck = 1;
const double Comm::_toRecv = 2.0;
const double Comm::_toAck = 2.0;

Comm::Comm() :
	_bPckInfoSet(false),
	_szDataMaxInit(_szBufTX - sizeof(_TXInit)),
	_szDataMaxPart(_szBufTX - sizeof(_TXPart)),
	_szDataMax(_szDataMaxInit + _szDataMaxPart * 253), // maximum number of packet segments (unsigned int = 256) - two last empty packet which are send to end communication
	_pRXBuf(new char[_szBufRX]),
	_pPublic(NULL),
	_pPrivate(NULL),
	_pRSAPvt(NULL),
	_pRSAPub(NULL),
	_pDecryptBuf(NULL),
	_pEncryptBuf(NULL),
	_szDecryptBuf(0),
	_szEncryptBuf(0),
	_szRSAPub(0),
	_szRSAPvt(0)
{
	_pChk = new char[_szDataMax];
	_pRXRsp = reinterpret_cast<PacketInfoRsp*>(_pRXBuf);
	_pRXInit = reinterpret_cast<PacketInfoInit*>(_pRXBuf);
	_pRXPart = reinterpret_cast<PacketInfoPart*>(_pRXBuf);

	memset(&_TXInit, 0, sizeof(_TXInit));
	memset(&_TXPart, 0, sizeof(_TXPart));
	memset(&_RXInfo, 0, sizeof(_RXInfo));
	memset(&_RXRsp, 0, sizeof(_RXRsp));
}

Comm::~Comm()
{
	delete[] _pChk;
	_releaseCrypt();
}

bool Comm::Init()
{
	bool okInit = _rn.Init();
	if (!okInit)
	{
		return false;
	}

	return true;
}

bool Comm::SetInfo(const PacketInfo *pInfo)
{
	if (!pInfo)
	{
		return false;
	}

	_RXRsp.LocalId = _RXInfo.LocalId = _TXPart.LocalId = _TXInit.LocalId = pInfo->LocalId;
	_RXRsp.RemoteId = _RXInfo.RemoteId = _TXPart.RemoteId = _TXInit.RemoteId = pInfo->RemoteId;
	_RXRsp.Port = _RXInfo.Port = _TXPart.Port = _TXInit.Port = pInfo->Port;
	_bPckInfoSet = true;

	return true;
}

bool Comm::SetCrypt(const char *pPublic, const char *pPrivate)
{
	if (pPublic == NULL || pPrivate == NULL)
	{
		_releaseCrypt();
		return true;
	}

	_pPublic = BIO_new_file(pPublic, "r");
	_pPrivate = BIO_new_file(pPrivate, "r");

	if (!(_pPublic && _pPrivate))
	{
		return false;
	}

	_pRSAPub = PEM_read_bio_RSAPublicKey(_pPublic, NULL, NULL, NULL);
	_pRSAPvt = PEM_read_bio_RSAPrivateKey(_pPrivate, NULL, NULL, NULL);
	
	if (!(_pRSAPub && _pRSAPvt))
	{
		return false;
	}

	_szRSAPub = RSA_size(_pRSAPub);
	_szRSAPvt = _szRSAPub - 12;

	unsigned int encryptPck = _szDataMax / _szRSAPub;
	_szEncryptBuf = encryptPck * _szRSAPub;
	_szDecryptBuf = encryptPck * _szRSAPvt;

	if (!(_szEncryptBuf && _szDecryptBuf))
	{
		return false;
	}

	_pEncryptBuf = new unsigned char[_szEncryptBuf];
	_pDecryptBuf = new unsigned char[_szDecryptBuf];

	if (!(_pEncryptBuf && _pDecryptBuf))
	{
		return false;
	}

	return true;
}

bool Comm::Send(const void *pData, size_t szData, bool ack)
{
	// check if data size is small enough to fit into TX buffer
	
	if (szData > _szDataMax)
	{
#ifdef _DEBUG_COMM_SR
		printf(RED "[ERROR]" WHITE " SEND size(%u), ack(%i)\n", szData, ack);
#endif
		return false;
	}

#ifdef _DEBUG_COMM_SR
	printf(GREEN "[OK]" WHITE " SEND START size(%u), ack(%i)\n", szData, ack);
#endif

	const char *ptr = static_cast<const char*>(pData);

	// set init packet info

	_TXInit.Ack = ack;
	_TXInit.SizeTotal = szData;
	_TXInit.Size = szData < _szDataMaxInit ? szData : _szDataMaxInit;

	// send init packet

	bool okTX = _send(&_TXInit, sizeof(_TXInit), ptr);

	if (!okTX)
	{
		// if not TX is not successfull raise error

#ifdef _DEBUG_COMM_SR
		printf(RED "[ERROR]" WHITE " SEND END size(0/%u), ack(%i)\n", szData, ack);
#endif
		return false;
	}

	size_t left = _TXInit.SizeTotal - _TXInit.Size;
	ptr += _TXInit.Size;

	_TXPart.SegId = 0;
	
	// send part packets
	
	while (left)
	{
		_TXPart.Size = left < _szDataMaxPart ? left : _szDataMaxPart;
		_TXPart.SegId++;

		okTX = _send(&_TXPart, sizeof(_TXPart), ptr);

		if (!okTX)
		{
#ifdef _DEBUG_COMM_SR
			printf(RED "[ERROR]" WHITE " SEND END size(%u/%u), ack(%i)\n", szData - left, szData, ack);
#endif
			return false;
		}

		ptr += _TXPart.Size;
		left -= _TXPart.Size;
	}

	// end communication and send empty packet if ack is requested
	
	if (ack)
	{	
		_TXPart.Size = 0;
		_TXPart.SegId++;

		okTX = _send(&_TXPart, sizeof(_TXPart), NULL);

		_TXPart.SegId++;

		okTX = _send(&_TXPart, sizeof(_TXPart), NULL);
	}

#ifdef _DEBUG_COMM_SR
	printf(GREEN "[OK]" WHITE " SEND END size(%u/%u), ack(%i)\n", szData - left, szData, ack);
#endif

	return true;
}

bool Comm::Receive(void *pData, size_t szData, size_t *pSzDataRX)
{
	char *ptr = static_cast<char*>(pData);
	size_t szLeft = szData;

#ifdef _DEBUG_COMM_SR
	printf(GREEN "[OK]" WHITE " RECEIVE START size(%u)\n", szData);
#endif

	_RXInfo.SegId = 0;

	// start receiving until all packets have been received
	
	do
	{
		bool okRX = _receive(ptr, szLeft);
		if (!okRX)
		{
#ifdef _DEBUG_COMM_SR
			printf(RED "[ERROR]" WHITE " RECEIVE END size(%u/%u)\n", szData, _RXInfo.SegId ? _RXInfo.SizeTotal : 0);
#endif
			if (pSzDataRX)
			{
				*pSzDataRX = _RXInfo.SegId ? _RXInfo.SizeTotal : 0;
			}

			return false;
		}

		// if correct packet segment is received

		if (_pRXPart->SegId == _RXInfo.SegId)
		{
			// if appropriate init packet is received

			if (!_RXInfo.SegId)
			{
				_RXInfo.SegId++;
				_RXInfo.SizeTotal = _pRXInit->SizeTotal;
				szLeft = (_RXInfo.SizeTotal > szData ? szData : _RXInfo.SizeTotal) - _pRXInit->Size;
				ptr += _pRXInit->Size;
			}

			// if appropriate part packet is received

			else
			{
				_RXInfo.SegId++;
				szLeft -= _pRXPart->Size;
				ptr += _pRXPart->Size;
			}
		}
		else
		{
			if (	_pRXPart->SegId + 1 == _RXInfo.SegId &&
				_RXInfo.Ack &&
				_RXInfo.SegId != 0)
			{
				continue;
			}
			else
			{
#ifdef _DEBUG_COMM_SR
				printf(RED "[ERROR]" WHITE " RECEIVE END size(%u/%u)\n", szData, _RXInfo.SegId ? _RXInfo.SizeTotal : 0);
#endif
				if (pSzDataRX)
				{
					*pSzDataRX = _RXInfo.SegId ? _RXInfo.SizeTotal : 0;
				}
				return false;
			}
		}
	} while (szLeft);

	// if ack is requested, receive last two packets which are used to signal end of communication

	if (_RXInfo.Ack)
	{
		_receive(NULL, 0);
		_receive(NULL, 0);
	}

	if (pSzDataRX)
	{
		*pSzDataRX = _RXInfo.SizeTotal;
	}

#ifdef _DEBUG_COMM_SR
	printf(	GREEN "[OK]" WHITE " RECEIVE END size(%u/%u)\n",
		(_RXInfo.SizeTotal > szData ? szData : _RXInfo.SizeTotal) - _pRXInit->Size,
		_RXInfo.SizeTotal);
#endif

	return true;
}

bool Comm::EncryptPubSend(const void *pData, size_t szData, bool ack)
{
	if (szData > _szDecryptBuf)
	{
#ifdef _DEBUG_CRYPT
		printf(RED "[ERROR]" WHITE " ENCRYPT PUB START decryptSize(%u), maxDecryptSize(%u)\n", szData, _szDecryptBuf);
#endif
		return false;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " ENCRYPT PUB START decryptSize(%u), maxDecryptSize(%u)\n", szData, _szDecryptBuf);
#endif

	const unsigned char *ptr = static_cast<const unsigned char*>(pData);
	const unsigned char *pFrom = ptr;
	unsigned char *pTo = _pEncryptBuf;

	size_t szLeft = szData;

	do
	{
		size_t sz = szLeft <= _szRSAPvt ? szLeft : _szRSAPvt;

		double time = _clk.Total();
		RAND_seed(&time, sizeof(double));
		int szEncrypted = RSA_public_encrypt(
			sz,
			pFrom,
			pTo,
			_pRSAPub,
			RSA_PKCS1_PADDING);

		if (szEncrypted == -1)
		{
#ifdef _DEBUG_CRYPT
			printf(RED "[ERROR]" WHITE " ENCRYPT PUB decryptSize(%u), encryptSize(0)\n", sz);
#endif
			return false;
		}

#ifdef _DEBUG_CRYPT
		printf(GREEN "[OK]" WHITE " ENCRYPT PUB decryptSize(%u), encryptSize(%i)\n", sz, szEncrypted);
#endif

		pFrom += sz;
		pTo += szEncrypted;
		szLeft = szData - (pFrom - ptr);
	} while(szLeft);

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " ENCRYPT PUB END decryptSize(%u), encryptSize(%u)\n", pFrom - ptr, pTo - _pEncryptBuf);
#endif

 	return Send(_pEncryptBuf, pTo - _pEncryptBuf, ack);
}

bool Comm::EncryptPvtSend(const void *pData, size_t szData, bool ack)
{
	if (szData > _szDecryptBuf)
	{
#ifdef _DEBUG_CRYPT
		printf(RED "[ERROR]" WHITE " ENCRYPT PVT START decryptSize(%u), maxDecryptSize(%u)\n", szData, _szDecryptBuf);
#endif
		return false;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " ENCRYPT PVT START decryptSize(%u), maxDecryptSize(%u)\n", szData, _szDecryptBuf);
#endif

	const unsigned char *ptr = static_cast<const unsigned char*>(pData);
	const unsigned char *pFrom = ptr;
	unsigned char *pTo = _pEncryptBuf;

	size_t szLeft = szData;

	do
	{
		size_t sz = szLeft <= _szRSAPvt ? szLeft : _szRSAPvt;

		int szEncrypted = RSA_private_encrypt(
			sz,
			pFrom,
			pTo,
			_pRSAPvt,
			RSA_PKCS1_PADDING);

		if (szEncrypted == -1)
		{
#ifdef _DEBUG_CRYPT
			printf(RED "[ERROR]" WHITE " ENCRYPT PVT decryptSize(%u), encryptSize(0)\n", sz);
#endif
			return false;
		}

#ifdef _DEBUG_CRYPT
		printf(GREEN "[OK]" WHITE " ENCRYPT PVT decryptSize(%u), encryptSize(%i)\n", sz, szEncrypted);
#endif

		pFrom += sz;
		pTo += szEncrypted;
		szLeft = szData - (pFrom - ptr); 
	} while(szLeft);

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " ENCRYPT PVT END decryptSize(%u), encryptSize(%u)\n", pFrom - ptr, pTo - _pEncryptBuf);
#endif

 	return Send(_pEncryptBuf, pTo - _pEncryptBuf, ack);
}

bool Comm::ReceiveDecryptPub(void *pData, size_t szData, size_t *pSzDataRX)
{
	size_t szLeft;
	bool okRX = Receive(_pEncryptBuf, _szEncryptBuf, &szLeft);

	if (!okRX)
	{
		if (pSzDataRX)
		{
			*pSzDataRX = 0;
		}
#ifdef _DEBUG_CRYPT
		printf(RED "[ERROR]" WHITE " DECRYPT PUB START encryptSize(%u), maxDecryptSize(%u)\n", szLeft, szData);
#endif
		return false;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " DECRYPT PUB START encryptSize(%u), maxDecryptSize(%u)\n", szLeft, szData);
#endif

	size_t szFree = szData;
	int szDecrypt;
	unsigned char *ptr = static_cast<unsigned char*>(pData);
	unsigned char *pFrom = _pEncryptBuf;
	unsigned char *pTo = ptr;

	do
	{
		size_t sz = szLeft > _szRSAPub ? _szRSAPub : szLeft;

		if (szFree >= _szRSAPvt)
		{
			szDecrypt = RSA_public_decrypt(
				sz,
				pFrom,
				pTo,
				_pRSAPub,
				RSA_PKCS1_PADDING);

			if (szDecrypt == -1)
			{
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PUB encryptSize(%u), decryptSize(0), available(%u)\n", sz, szFree);
#endif
				return false;
			}

#ifdef _DEBUG_CRYPT
			printf(GREEN "[OK]" WHITE " DECRYPT PUB encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
		}
		else
		{
			szDecrypt = RSA_public_decrypt(
				sz,
				pFrom,
				_pDecryptBuf,
				_pRSAPub,
				RSA_PKCS1_PADDING);

			if (szDecrypt == -1)
			{
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PUB encryptSize(%u), decryptSize(0), available(%u)\n", sz, szFree);
#endif
				return false;
			}
		
			if (szFree > szDecrypt)
			{
				memcpy(pTo, _pDecryptBuf, szDecrypt);
#ifdef _DEBUG_CRYPT
				printf(GREEN "[OK]" WHITE " DECRYPT PUB encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
			}
			else
			{
				memcpy(pTo, _pDecryptBuf, szFree);
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr + szFree;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PUB encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
				return false;
			}
		}

		pFrom += sz;
		pTo += szDecrypt;
		szLeft -= sz;
		szFree -= szDecrypt;
	} while (szLeft);


	if (pSzDataRX)
	{
		*pSzDataRX = pTo - ptr;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " DECRYPT PUB END encryptSize(%u), decryptSize(%u)\n", pFrom - _pEncryptBuf, pTo - ptr);
#endif

 	return true; 
}

bool Comm::ReceiveDecryptPvt(void *pData, size_t szData, size_t *pSzDataRX)
{
	size_t szLeft;
	bool okRX = Receive(_pEncryptBuf, _szEncryptBuf, &szLeft);

	if (!okRX)
	{
		if (pSzDataRX)
		{
			*pSzDataRX = 0;
		}
#ifdef _DEBUG_CRYPT
		printf(RED "[ERROR]" WHITE " DECRYPT PVT START encryptSize(%u), maxDecryptSize(%u)\n", szLeft, szData);
#endif
		return false;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " DECRYPT PVT START encryptSize(%u), maxDecryptSize(%u)\n", szLeft, szData);
#endif

	size_t szFree = szData;
	int szDecrypt;
	unsigned char *ptr = static_cast<unsigned char*>(pData);
	unsigned char *pFrom = _pEncryptBuf;
	unsigned char *pTo = ptr;

	do
	{
		size_t sz = szLeft > _szRSAPub ? _szRSAPub : szLeft;

		if (szFree >= _szRSAPvt)
		{
			szDecrypt = RSA_private_decrypt(
				sz,
				pFrom,
				pTo,
				_pRSAPvt,
				RSA_PKCS1_PADDING);

			if (szDecrypt == -1)
			{
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PVT encryptSize(%u), decryptSize(0), available(%u)\n", sz, szFree);
#endif
				return false;
			}
#ifdef _DEBUG_CRYPT
			printf(GREEN "[OK]" WHITE " DECRYPT PVT encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
		}
		else
		{
			szDecrypt = RSA_private_decrypt(
				sz,
				pFrom,
				_pDecryptBuf,
				_pRSAPvt,
				RSA_PKCS1_PADDING);

			if (szDecrypt == -1)
			{
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PVT encryptSize(%u), decryptSize(0), available(%u)\n", sz, szFree);
#endif
				return false;
			}

			if (szFree > szDecrypt)
			{
				memcpy(pTo, _pDecryptBuf, szDecrypt);
#ifdef _DEBUG_CRYPT
				printf(GREEN "[OK]" WHITE " DECRYPT PVT encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
			}
			else
			{
				memcpy(pTo, _pDecryptBuf, szFree);
				if (pSzDataRX)
				{
					*pSzDataRX = pTo - ptr + szFree;
				}
#ifdef _DEBUG_CRYPT
				printf(RED "[ERROR]" WHITE " DECRYPT PVT encryptSize(%u), decryptSize(%i), available(%u)\n", sz, szDecrypt, szFree);
#endif
				return false;
			}
		}

		pFrom += sz;
		pTo += szDecrypt;
		szLeft -= sz;
		szFree -= szDecrypt;
		
	} while (szLeft);

	if (pSzDataRX)
	{
		*pSzDataRX = pTo - ptr;
	}

#ifdef _DEBUG_CRYPT
	printf(GREEN "[OK]" WHITE " DECRYPT PUB END encryptSize(%u), decryptSize(%u)\n", pFrom - _pEncryptBuf, pTo - ptr);
#endif

 	return true; 
}

size_t Comm::GetMaxSz() { return _szDataMax; }

size_t Comm::GetSzEncryptBuf() { return _szEncryptBuf; }

size_t Comm::GetSzDecryptBuf() { return _szDecryptBuf; }

bool Comm::_send(const PacketInfoPart *pInfo, size_t szInfo, const char *pData)
{
	const char *pInfoUC = reinterpret_cast<const char*>(pInfo);

	char retrySend = 0;
	bool resend;
	do
	{
		if (retrySend++ > _retrySend)
		{
			// maximum number of retries reached, raise error
			
#ifdef _DEBUG_COMM_TR
			if (szInfo == sizeof(PacketInfoInit))
			{
				printf(	RED "[ERROR]" WHITE " TX(%i) INIT, attempt(%i/%i), ack(%i), size(%u/%u)\n",
					pInfo->SegId, retrySend, _retrySend, _TXInit.Ack, pInfo->Size,
					static_cast<const PacketInfoInit*>(pInfo)->SizeTotal);
			}
			else
			{
				printf(RED "[ERROR]" WHITE " TX(%i) PART, attempt(%i/%i), ack(%i), size(%u)\n",
					pInfo->SegId, retrySend, _retrySend, _TXInit.Ack, pInfo->Size);
			}
#endif

			return false;
		}

		bool okTX;

		char retryTX = 0;
		do
		{
			if (retryTX++ > _retryTX)
			{
				// maximum number of retries reached, raise error

#ifdef _DEBUG_COMM_TR
			if (szInfo == sizeof(PacketInfoInit))
			{
				printf(	RED "[ERROR]" WHITE " TX(%i) INIT, attemptTX(%i/%i), ack(%i), size(%u/%u)\n",
					pInfo->SegId, retryTX, _retryTX, _TXInit.Ack, pInfo->Size,
					static_cast<const PacketInfoInit*>(pInfo)->SizeTotal);
			}
			else
			{
				printf(RED "[ERROR]" WHITE " TX(%i) PART, attemptTX(%i/%i), ack(%i), size(%u)\n",
					pInfo->SegId, retryTX, _retryTX, _TXInit.Ack, pInfo->Size);
			}
#endif
				return false;
			}

			// try to send data
			
			if (pInfo->Size)
			{
				okTX = _rn.TX(pInfoUC, szInfo, pData, pInfo->Size);
			}
			else
			{
				okTX = _rn.TX(pInfoUC, szInfo);
			}

#ifdef _DEBUG_COMM_TR
			if (szInfo == sizeof(PacketInfoInit))
			{
				cout << (okTX ? GREEN "[OK]" WHITE : RED "[ERROR]" WHITE);
				printf(	" TX(%i) INIT, attempt(%i), attemptTX(%i), ack(%i), size(%u/%u)\n",
					pInfo->SegId, retrySend, retryTX, _TXInit.Ack, pInfo->Size,
					static_cast<const PacketInfoInit*>(pInfo)->SizeTotal);
			}
			else
			{
				cout << (okTX ? GREEN "[OK]" WHITE : RED "[ERROR]" WHITE);
				printf(	" TX(%i) PART, attempt(%i), attemptTX(%i), ack(%i), size(%u)\n",
					pInfo->SegId, retrySend, retryTX, _TXInit.Ack, pInfo->Size);
			}
#endif
		} while (!okTX);

		// if data is successfully sent, wait for ack packet if requested
		
		if (_TXInit.Ack)
		{
			// reset timeout counter
			
			_clk.Reset();

			// exit following loop only if appropriate response has been received or timeout occured

			bool timeout = false;
			double time;
			do
			{
				size_t szRX;
				do
				{
					// if timeout is reached

					time = _clk.Now();
					if (time > _toAck)
					{
						timeout = true;
						continue;
					}

					// receive ack

					szRX = _rn.RX(
						reinterpret_cast<char*>(_pRXRsp),
						sizeof(*_pRXRsp));
				} while (szRX != sizeof(*_pRXRsp) && !timeout);
			} while (!(_checkInfo(&_RXRsp, _pRXRsp)) && !timeout);
			
			resend = timeout ? timeout : (_pRXRsp->RequestResend || _pRXRsp->SegId != pInfo->SegId);

#ifdef _DEBUG_COMM_TR
			if (timeout)
			{
				printf(BROWN "[WARNING]" WHITE " ACK timeout(%f)\n", time);
			}
			else if (resend)
			{
				printf(	BROWN "[WARNING]" WHITE " ACK(%i) seg(%i), requestResend(%i)\n",
					pInfo->SegId, _pRXRsp->SegId, _pRXRsp->RequestResend);
			}
			else
			{
				printf(	GREEN "[OK]" WHITE " ACK(%i) seg(%i), requestResend(%i)\n",
					pInfo->SegId, _pRXRsp->SegId, _pRXRsp->RequestResend);
			}
#endif
		}
		else
		{
			resend = false;
		}
	} while (resend);

	return true;
}

bool Comm::_sendAck()
{
	char retryTXAck = 0;
	bool okTX;
	do
	{
		if (retryTXAck++ > _retryTXAck)
		{
			// maximum number of retries reached, raise error
			
#ifdef _DEBUG_COMM_TR
			printf(RED "[ERROR]" WHITE " ACK attempt(%i/%i)\n", retryTXAck, _retryTXAck);
#endif

			return false;
		}

		okTX = _rn.TX(
			reinterpret_cast<char*>(&_RXRsp),
			sizeof(_RXRsp));
#ifdef _DEBUG_COMM_TR
		cout << (okTX ? GREEN "[OK]" WHITE : RED "[ERROR]" WHITE);
		printf(	" ACK(%i) attempt(%i/%i), requestResend(%i)\n", _RXRsp.SegId, retryTXAck, _retryTXAck, _RXRsp.RequestResend);
#endif
	} while (!okTX);

	return true;
}

bool Comm::_receive(char *pData, size_t szData)
{
	// reset timeout counter
	
	_clk.Reset();
	
	size_t szRX;

	// start receiving until appropriate packet is received
	
	bool retryRX;
	do
	{
		do
		{
			do
			{
				szRX = _rn.RX(_pRXBuf, _szBufRX);

				// if timeout is reached
				
				double time = _clk.Now();				
				if (time > _toRecv)
				{
#ifdef _DEBUG_COMM_TR
					printf(RED "[ERROR]" WHITE " RX timeout(%f/%f)\n", time, _toRecv);
#endif
					return false;
				}
			} while (szRX < sizeof(*_pRXPart));
		} while (!_checkInfo(&_RXInfo, _pRXPart));

		// determine size of info packet
		
		size_t szInfo;

		if (_pRXPart->SegId)
		{
			szInfo = sizeof(*_pRXPart);
		}
		else
		{
			szInfo = sizeof(*_pRXInit);
			_RXInfo.Ack = _pRXInit->Ack;
		}

		// determine if size of received packet is ok
		
		bool okRX = szRX == _pRXPart->Size + szInfo;

		// copy data to pData and reply with ack packet if requested
		
		if (okRX)
		{
#ifdef _DEBUG_COMM_TR
			if (_pRXPart->SegId)
			{
				printf(GREEN "[OK]" WHITE " RX(%i) part, size(%u)\n", _pRXPart->SegId, _pRXPart->Size);
			}
			else
			{
				printf(GREEN "[OK]" WHITE " RX(%i) part, ack(%i), size(%u/%u)\n", _pRXInit->SegId, _pRXInit->Ack, _pRXInit->Size, _pRXInit->SizeTotal);
			}
#endif
			retryRX = false;

			if (_pRXPart->Size)
			{
				size_t szCopy = szData > _pRXPart->Size ? _pRXPart->Size : szData;
				memcpy(pData, _pRXBuf + szInfo, szCopy);
			}

			if (_RXInfo.Ack)
			{
				_RXRsp.RequestResend = false;
				_RXRsp.SegId = _pRXPart->SegId;
				bool okTX = _sendAck();
				if (!okTX)
				{
					return false;
				}
			}
		}
		else
		{
#ifdef _DEBUG_COMM_TR
			if (_pRXPart->SegId)
			{
				cout << (_RXInfo.Ack ? BROWN "[WARNING]" WHITE : RED "[ERROR]" WHITE);
				printf(	" RX(%i) part, size(%u/%u)\n",
					_pRXPart->SegId, _pRXPart->Size, szRX - szInfo);
			}
			else
			{
				cout << (_RXInfo.Ack ? BROWN "[WARNING]" WHITE : RED "[ERROR]" WHITE);
				printf(	" RX(%i) part, ack(%i), size(%u/%u)\n",
					_pRXInit->SegId, _pRXInit->Ack, _pRXInit->Size, szRX - szInfo);
			}
#endif
			if (_RXInfo.Ack)
			{
				retryRX = true;
				_RXRsp.RequestResend = true;
				_RXRsp.SegId = _pRXPart->SegId;
				bool okTX = _sendAck();
				if (!okTX)
				{
					return false;
				}
			}
			else
			{
				// if packet is wrong and ack is not requested raise error
				
				return false;
			}
		}
	} while (retryRX);

	return true;
}

bool Comm::_checkInfo(const PacketInfo *pInfoA, const PacketInfo *pInfoB)
{
	return	pInfoA->LocalId == pInfoB->RemoteId &&
		pInfoA->RemoteId == pInfoB->LocalId &&
		pInfoA->Port == pInfoB->Port;
}

void Comm::_releaseCrypt()
{
	_szRSAPvt = 0;
	_szRSAPub = 0;
	_szEncryptBuf = 0;
	_szDecryptBuf = 0;

	if (_pRSAPvt)
	{
		RSA_free(_pRSAPvt);
		_pRSAPvt = NULL;
	}

	if (_pRSAPub)
	{
		RSA_free(_pRSAPub);
		_pRSAPub = NULL;
	}

	if (_pPublic)
	{
		BIO_free_all(_pPublic);
		_pPublic = NULL;
	}

	if (_pPrivate)
	{
		BIO_free_all(_pPrivate);
		_pPrivate = NULL;
	}

	if (_pEncryptBuf)
	{
		delete[] _pEncryptBuf;
		_pEncryptBuf = NULL;
	}

	if (_pDecryptBuf)
	{
		delete[] _pDecryptBuf;
		_pDecryptBuf = NULL;
	}
}
