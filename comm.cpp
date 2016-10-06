#include "comm.h"

using namespace RN;

const size_t Comm::_szBufTX = 63;
const size_t Comm::_szBufRX = 63;

Comm::Comm() :
	_bPckInfoSet(false),
	_szDataMaxInit(_szBufTX - sizeof(_TXInit)),
	_szDataMaxPart(_szBufTX - sizeof(_TXPart)),
	_pRXBuf(new char[_szBufRX])
{
	_pRXRsp = reinterpret_cast<PacketInfoRsp*>(_pRXBuf);
	_pRXInit = reinterpret_cast<PacketInfoInit*>(_pRXBuf);
	_pRXPart = reinterpret_cast<PacketInfoPart*>(_pRXBuf);
}

Comm::~Comm()
{
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

	_RXRsp.FromId = _RXInit.FromId = _TXPart.FromId = _TXInit.FromId = pInfo->FromId;
	_RXRsp.ToId = _RXInit.ToId = _TXPart.ToId = _TXInit.ToId = pInfo->ToId;
	_RXRsp.Port = _RXInit.Port = _TXPart.Port = _TXInit.Port = pInfo->Port;
	_bPckInfoSet = true;

	return true;
}

bool Comm::Send(const void *pData, size_t szData)
{
	const char *ptr = reinterpret_cast<const char*>(pData);
	if (!_bPckInfoSet)
	{
		return false;
	}

	_TXInit.SegId = 0;
	_TXInit.Ack = false;
	_TXInit.SizeTotal = szData;
	_TXInit.Size = _szDataMaxInit < szData ? _szDataMaxInit : szData;

	printf("TX (%i B)\r\n", _TXInit.SizeTotal);
	
	bool okTX =
		_rn.TX(	reinterpret_cast<char*>(&_TXInit),
			sizeof(_TXInit),
			ptr,
			_TXInit.Size);

	if (!okTX)
	{
		return false;
	}

	printf("Init pkg TX%i (%i B)\r\n", _TXInit.SegId, _TXInit.Size);

	size_t szSent = _TXInit.Size;
	_TXPart.SegId = 1;
	while (szSent < szData)
	{
		_TXPart.Size = _szDataMaxPart < szData - szSent ? _szDataMaxPart : szData - szSent;

		bool okTX = _rn.TX(
				reinterpret_cast<char*>(&_TXPart),
				sizeof(_TXPart),
				ptr + szSent,
				_TXPart.Size);

		if (!okTX)
		{
			return false;
		}

		printf("Part pkg TX%i (%i B)\r\n", _TXPart.SegId, _TXPart.Size);

		szSent += _TXPart.Size;
		_TXPart.SegId++;
	}

	return true;
}


bool Comm::SendAck(const void *pData, size_t szData)
{
	const char *ptr = reinterpret_cast<const char*>(pData);
	if (!_bPckInfoSet)
	{
		return false;
	}

	_TXInit.SegId = 0;
	_TXInit.Ack = true;
	_TXInit.SizeTotal = szData;
	_TXInit.Size = _szDataMaxInit < szData ? _szDataMaxInit : szData;

	printf("TX (%i B)\r\n", _TXInit.SizeTotal);
	
	ResponseStatus status;
	do
	{
		bool okTX =
			_rn.TX(	reinterpret_cast<char*>(&_TXInit),
				sizeof(_TXInit),
				ptr,
				_TXInit.Size);

		if (!okTX)
		{
			return false;
		}
	
		do
		{
			size_t szRX;
			do
			{
				szRX = _rn.RX(reinterpret_cast<char*>(_pRXBuf), _szBufRX);
			} while (szRX != sizeof(*_pRXRsp));
			status = _okResInit();
		} while (status == RSWRONG);
	} while (status == RSRESEND);

	printf("Init pkg TX%i (%i B)\r\n", _TXInit.SegId, _TXInit.Size);

	size_t szSent = _TXInit.Size;
	_TXPart.SegId = 1;
	while (szSent < szData)
	{
		_TXPart.Size = _szDataMaxPart < szData - szSent ? _szDataMaxPart : szData - szSent;

		do
		{
			bool okTX = _rn.TX(
					reinterpret_cast<char*>(&_TXPart),
					sizeof(_TXPart),
					ptr + szSent,
					_TXPart.Size);

			if (!okTX)
			{
				return false;
			}
			
			do
			{
				size_t szRX;
				do
				{
					szRX = _rn.RX(reinterpret_cast<char*>(_pRXBuf), _szBufRX);
				} while (szRX != sizeof(*_pRXRsp));
				status = _okResPart();
			} while (status == RSWRONG);
		} while (status == RSRESEND);

		printf("Part pkg TX%i (%i B)\r\n", _TXPart.SegId, _TXPart.Size);

		szSent += _TXPart.Size;
		_TXPart.SegId++;
	}

	return true;
}

bool Comm::Receive(void *pData, size_t szData)
{
	char *ptr = reinterpret_cast<char*>(pData);
	_pRXData = _pRXBuf + sizeof(_RXInit);

	size_t szRead;
	size_t szRX;
	bool okRX = false;

	_RXInit.SegId = 0;
	do
	{
		do
		{
			do
			{
				szRX = _rn.RX(_pRXBuf, _szBufRX);
			} while (szRX < sizeof(_RXInit));
		} while (_okRXInit() == RSWRONG);

		_RXInit.SizeTotal = _pRXInit->SizeTotal;
		_RXInit.Ack = _pRXInit->Ack;
		szRead = szRX - sizeof(*_pRXInit);

		okRX = szRead == _pRXInit->Size;
			

		if (_RXInit.Ack)
		{
			_RXRsp.RequestResend = !okRX;
			_RXRsp.SegId = 0;
			
			bool okTX = _rn.TX(
					reinterpret_cast<char*>(&_RXRsp),
					sizeof(_RXRsp));

			if (!okTX)
			{
				return false;
			}
		}
		else
		{
			if (!okRX)
			{
				return false;
			}
		}
	} while (!okRX && _RXInit.Ack);

	printf("RX (%i B)\r\n", _pRXInit->SizeTotal);
	printf("Init pkg RX%i (%i B)\r\n", _pRXInit->SegId, _pRXInit->Size);

	if (szRead > szData)
	{
		memcpy(ptr, _pRXData, szData);
		return false;
	}

	memcpy(ptr, _pRXData, szRead);

	_pRXData = _pRXBuf + sizeof(*_pRXPart);

	while (szRead < _RXInit.SizeTotal)
	{
		_RXInit.SegId++;
		do
		{
			do
			{
				do
				{
					szRX = _rn.RX(_pRXBuf, _szBufRX);
				} while (szRX < sizeof(*_pRXPart));
			} while (_okRXPart() == RSWRONG);

			_RXInit.Size = szRX - sizeof(*_pRXPart);
			okRX = _RXInit.Size == _pRXPart->Size;

			if (_RXInit.Ack)
			{
				_RXRsp.RequestResend = !okRX;
				_RXRsp.SegId = _pRXPart->SegId;
				
				bool okTX = _rn.TX(
						reinterpret_cast<char*>(&_RXRsp),
						sizeof(_RXRsp));

				if (!okTX)
				{
					return false;
				}
			}
			else
			{
				if (!okRX)
				{
					return false;
				}
			}
		} while (!okRX && _RXInit.Ack);

		if (szRead + _RXInit.Size > szData)
		{
			memcpy(ptr + szRead, _pRXData, szData - szRead);
			return false;
		}

		memcpy(ptr + szRead, _pRXData, _RXInit.Size);
		szRead += _RXInit.Size;
		
		printf("Init pkg RX%i (%i B)\r\n", _pRXPart->SegId, _pRXPart->Size);
	}

	return true;
}

ResponseStatus Comm::_okResInit()
{
	if (	_TXInit.FromId == _pRXRsp->FromId &&
		_TXInit.ToId == _pRXRsp->ToId &&
		_TXInit.Port == _pRXRsp->Port &&
		_TXInit.SegId == _pRXRsp->SegId)
	{
		if (_pRXRsp->RequestResend)
		{
			return RSRESEND;
		}
		else
		{
			return RSOK;
		}
	}
	else
	{
		return RSWRONG;
	}
}

ResponseStatus Comm::_okResPart()
{
	if (	_TXPart.FromId == _pRXRsp->FromId &&
		_TXPart.ToId == _pRXRsp->ToId &&
		_TXPart.Port == _pRXRsp->Port &&
		_TXPart.SegId == _pRXRsp->SegId)
	{
		if (_pRXRsp->RequestResend)
		{
			return RSRESEND;
		}
		else
		{
			return RSOK;
		}
	}
	else
	{
		return RSWRONG;
	}
}

ResponseStatus Comm::_okRXInit()
{
	if (	_RXInit.FromId == _pRXInit->FromId &&
		_RXInit.ToId == _pRXInit->ToId &&
		_RXInit.Port == _pRXInit->Port &&
		_RXInit.SegId == _pRXInit->SegId)
	{
		return RSOK;
	}
	else
	{
		return RSWRONG;
	}
}

ResponseStatus Comm::_okRXPart()
{
	if (	_RXInit.FromId == _pRXPart->FromId &&
		_RXInit.ToId == _pRXPart->ToId &&
		_RXInit.Port == _pRXPart->Port &&
		_RXInit.SegId == _pRXPart->SegId)
	{
		return RSOK;
	}
	else
	{
		return RSWRONG;
	}
}
