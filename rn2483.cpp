#include "rn2483.h"
#include <string>

using namespace std;
using namespace RN;

const size_t RN2483::_szBuf = 1024;

const char RN2483::_DNULL[] = "\0\0";
const char RN2483::_UVER[] = "Usys get ver\r\n";
const char RN2483::_MACPAUSE[] = "mac pause\r\n";
const char RN2483::_MACRESUME[] = "mac resume\r\n";
const char RN2483::_OK[] = "ok\r\n";
const char RN2483::_TXS[] = "radio tx ";
const char RN2483::_TXOK[] = "radio_tx_ok\r\n";
const char RN2483::_RX[] = "radio rx 0\r\n";
const char RN2483::_RXR[] = "radio_rx  ";
const char RN2483::_RXE[] = "radio_err\r\n";
const char RN2483::_RESET[] = "sys reset\r\n";
const char RN2483::_RESET_ACK[] = "RN2483";
const char RN2483::_END[] = "\r\n";
const char RN2483::_GETMOD[] = "radio get mod\r\n";
const char RN2483::_GETMODFSK[] = "fsk\r\n";
const char RN2483::_GETMODLORA[] = "lora\r\n";
const char RN2483::_SETMODFSK[] = "radio set mod fsk\r\n";
const char RN2483::_SETMODLORA[] = "radio set mod lora\r\n";
const char RN2483::_SETFREQ[] = "radio set freq ";
const char RN2483::_GETFREQ[] = "radio get freq\r\n";
const char RN2483::_SETPWR[] = "radio set pwr ";
const char RN2483::_GETPWR[]= "radio get pwr\r\n";
const char RN2483::_SETRATE[] = "radio set bitrate ";
const char RN2483::_GETRATE[] = "radio get bitrate\r\n";
const char RN2483::_SETDS[] = "radio set bt ";
const char RN2483::_GETDS[] = "radio get bt\r\n";
const char RN2483::_SETPRLEN[] = "radio set prlen ";
const char RN2483::_GETPRLEN[] = "radio get prlen\r\n";
const char RN2483::_SETCRCON[] = "radio set crc on\r\n";
const char RN2483::_SETCRCOFF[] = "radio set crc off\r\n";
const char RN2483::_GETCRC[] = "radio get crc\r\n";
const char RN2483::_SETRXBW[] = "radio set rxbw ";
const char RN2483::_GETRXBW[] = "radio get rxbw\r\n";
const char RN2483::_SETWDT[] = "radio set wdt ";
const char RN2483::_GETWDT[] = "radio get wdt\r\n";
const char RN2483::_SETSYNC[] = "radio set sync ";
const char RN2483::_GETSYNC[] = "radio get sync\r\n";
const char RN2483::_INVLD[] = "invalid param\r\n";

RN2483::RN2483() :
	_fd(-1),
	_pTX(new char[_szBuf + 1]),
	_pRX(new char[_szBuf + 1])
{
}

RN2483::~RN2483()
{
	close(_fd);
	delete[] _pTX;
	delete[] _pRX;
}

bool RN2483::Init()
{
	_fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
	if (_fd == -1)
	{
		return false;
	}
	
	int rc;
	struct termios options;
	memset (&options, 0, sizeof(options));
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_cflag = CS8 | CLOCAL | CREAD;
	options.c_lflag = ICANON;
	options.c_cc[VEOL] = '\n';

	rc = cfsetspeed(&options, B57600);
	if (rc != 0)
	{
		return false;
	}

	rc = tcflush(_fd, TCIOFLUSH);
	if (rc != 0)
	{
		return false;
	}

	rc = tcsetattr(_fd, TCSANOW, &options);
	if (rc != 0)
	{
		return false;
	}

	_write(_DNULL, sizeof(_DNULL) - 1);

	rc = cfsetspeed(&options, 57600);
	if (rc != 0)
	{
		return false;
	}

	rc = tcflush(_fd, TCIOFLUSH);
	if (rc != 0)
	{
		return false;
	}

	rc = tcsetattr(_fd, TCSANOW, &options);
	if (rc != 0)
	{
		return false;
	}

	_write(_UVER, sizeof(_UVER) - 1);
	_read(_pRX, _szBuf);
	// reset device
	bool rst = Reset();
	if (!rst)
	{
		rst = Reset();
		if (!rst)
		{
			return false;
		}
	}

	// set LORAWAN stack to off
	unsigned int  macPause = SetMACPause();
	if (!macPause)
	{
		return false;
	}

	// set modulation to FSK
	Mod modulation;
	bool okMod = SetMod(Mod::RNFSK);
	if (!okMod)
	{
		return false;
	}
	okMod = GetMod(&modulation);
	if (!okMod)
	{
		return false;
	}

	// set frequency for modulation
	bool okFreq = SetFreq(863500000);
	if (!okFreq)
	{
		return false;
	}

	unsigned int freq;
	okFreq = GetFreq(&freq);
	if (!okFreq)
	{
		return false;
	}

	// set power
	bool okPwr = SetPower(5);
	if (!okPwr)
	{
		return false;
	}

	char pwr;
	okPwr = GetPower(&pwr);
	if (!okPwr)
	{
		return false;
	}

	// set bit rate
	bool okRate = SetBitRate(2500);
	if (!okRate)
	{
		return false;
	}

	unsigned int rate;
	okRate = GetBitRate(&rate);
	if (!okRate)
	{
		return false;
	}

	// set data shaping parameter for FSK
	bool okBT = SetDataShaping(RNDS0_3);
	if (!okBT)
	{
		return false;
	}

	DataShaping bt;
	okBT = GetDataShaping(&bt);
	if (!okBT)
	{
		return false;
	}

	// set preamble length
	bool okPL = SetPreambleLength(8);
	if (!okPL)
	{
		return false;
	}

	unsigned int len;
	okPL = GetPreambleLength(&len);
	if (!okPL)
	{
		return false;
	}

	// set CRC off
	bool okCRC = SetCRC(false);
	if (!okCRC)
	{
		return false;
	}

	bool crc;
	okCRC = GetCRC(&crc);
	if (!okCRC)
	{
		return false;
	}

	// set RX bandwidth
	bool okRXBW = SetRXBW(RNRXBW12_5);
	if (!okRXBW)
	{
		return false;
	}

	RXBandWidth rxbw;
	okRXBW = GetRXBW(&rxbw);
	if (!okRXBW)
	{
		return false;
	}

	// set watch dog timeout
	bool okWDT = SetWDT(5000);
	if (!okWDT)
	{
		return false;
	}

	unsigned int wdt;
	okWDT = GetWDT(&wdt);
	if (!okWDT)
	{
		return false;
	}

	// set sync word
	bool okSync = SetSync("12");
	if (!okSync)
	{
		return false;
	}
	char sync[9];
	okSync = GetSync(sync, sizeof(sync) - 1);
	if (!okSync)
	{
		return false;
	}

	return true;
}

bool RN2483::TX(const void *ptr, size_t sz)
{
	// translate data
	size_t szHex = D2H(static_cast<const char*>(ptr), sz, _pTX);

	// transmit data
	bool okWrite = _write(_TXS, sizeof(_TXS) - 1);
	if (!okWrite)
	{
		return false;
	}

	okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	okWrite = _write(_END, sizeof(_END) - 1);
	if (!okWrite)
	{
		return false;
	}
	// receive ok status
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';
	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}
	szRead = _read(_pRX, _szBuf);
	if (szRead <= 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _TXOK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::TX(const void *ptr1, size_t sz1, const void *ptr2, size_t sz2)
{
	// transmit data
	bool okWrite = _write(_TXS, sizeof(_TXS) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	// translate and send first data segment
	size_t szHex = D2H(static_cast<const char*>(ptr1), sz1, _pTX);
	okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	// translate and send second data segment
	szHex = D2H(static_cast<const char*>(ptr2), sz2, _pTX);
	okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	okWrite = _write(_END, sizeof(_END) - 1);
	if (!okWrite)
	{
		return false;
	}
	// receive ok status
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';
	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}
	szRead = _read(_pRX, _szBuf);
	if (szRead <= 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _TXOK) != 0)
	{
		return false;
	}

	return true;
}

size_t RN2483::RX(void *pDst, size_t szDst)
{
	bool okWrite = _write(_RX, sizeof(_RX) - 1);
	if (!okWrite)
	{
		return 0;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return 0;
	}
	_pRX[szRead] = '\0';
	if (bcmp(_pRX, _OK) != 0)
	{
		return 0;
	}
	szRead = _read(_pRX, _szBuf);
	if (szRead <= 0)
	{
		return 0;
	}
	_pRX[szRead] = '\0';

	// if data is received
	if (bcmp(_pRX, _RXR) == 0)
	{
		return H2D(
			_pRX + sizeof(_RXR) - 1,
			szRead - (sizeof(_RXR) - 1) - 2,
			static_cast<char*>(pDst),
			szDst);
	}
	else if (bcmp(_pRX, _RXE) == 0)
	{
		return 0;
	}

	return 0;
}

unsigned int RN2483::SetMACPause()
{
	bool okWrite = _write(_MACPAUSE, sizeof(_MACPAUSE) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';
	
	unsigned int time = 0;
	int retrieved = sscanf(_pRX, "%u", &time);

	if (retrieved != 1)
	{
		return 0;
	}

	return time;
}

bool RN2483::SetMACResume()
{
	bool okWrite = _write(_MACRESUME, sizeof(_MACRESUME) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';
	
	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::Reset()
{
	bool okWrite = _write(_RESET, sizeof(_RESET) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _RESET_ACK) != 0)
	{
		return false;
	}
	return true;	
}

bool RN2483::SetMod(Mod modulation)
{
	bool okWrite;
	if (modulation == Mod::RNLORA)
	{
		okWrite = _write(_SETMODLORA, sizeof(_SETMODLORA) - 1);
	}
	else
	{
		okWrite = _write(_SETMODFSK, sizeof(_SETMODFSK) - 1);
	}
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
	}

	return true;
}

bool RN2483::GetMod(Mod *pModulation)
{
	bool okWrite = _write(_GETMOD, sizeof(_GETMOD) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _GETMODFSK) == 0)
	{
		*pModulation = Mod::RNFSK;
		return true;
	}
	else if (bcmp(_pRX, _GETMODLORA) == 0)
	{
		*pModulation = Mod::RNLORA;
		return true;
	}

	return false;
}

bool RN2483::SetFreq(unsigned int freq)
{
	bool okWrite = _write(_SETFREQ, sizeof(_SETFREQ) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	int i = sprintf(_pTX, "%u\r\n", freq);
	if (i <= 0)
	{
		return false;
	}

	okWrite = _write(_pTX, i);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetFreq(unsigned int *pFreq)
{
	bool okWrite = _write(_GETFREQ, sizeof(_GETFREQ) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	unsigned int freq;
	int i = sscanf(_pRX, "%u", &freq);
	if (i != 1)
	{
		return false;
	}
	*pFreq = freq;
	return true;
}

bool RN2483::SetPower(char power)
{
	bool okWrite = _write(_SETPWR, sizeof(_SETPWR) - 1);
	if (!okWrite)
	{
		return false;
	}
	int i = sprintf(_pTX, "%hhi\r\n", power);
	if (i <= 0)
	{
		return false;
	}

	okWrite = _write(_pTX, i);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetPower(char *pPower)
{
	bool okWrite = _write(_GETPWR, sizeof(_GETPWR) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	char pwr;
	int i = sscanf(_pRX, "%hhi", &pwr);
	if (i != 1)
	{
		return false;
	}

	*pPower = pwr;

	return true;
}

bool RN2483::SetBitRate(unsigned int rate)
{
	bool okWrite = _write(_SETRATE, sizeof(_SETRATE) - 1);
	if (!okWrite)
	{
		return false;
	}
	int i = sprintf(_pTX, "%u\r\n", rate);
	if (i <= 0)
	{
		return false;
	}

	okWrite = _write(_pTX, i);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetBitRate(unsigned int *pRate)
{
	bool okWrite = _write(_GETRATE, sizeof(_GETRATE) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	unsigned int rate;
	int i = sscanf(_pRX, "%u", &rate);
	if (i != 1)
	{
		return false;
	}

	*pRate = rate;

	return true;
}

bool RN2483::SetDataShaping(DataShaping param)
{
	bool okWrite = _write(_SETDS, sizeof(_SETDS) - 1);
	if (!okWrite)
	{
		return false;
	}

	switch (param)
	{
		case RNDS1_0:
			_write("1.0\r\n", sizeof("1.0\r\n") - 1);
			break;
		case RNDS0_5:
			_write("0.5\r\n", sizeof("0.5\r\n") - 1);
			break;
		case RNDS0_3:
			_write("0.3\r\n", sizeof("0.3\r\n") - 1);
			break;
		case RNDSNone:
			_write("none\r\n", sizeof("none\r\n") - 1);
			break;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetDataShaping(DataShaping *pParam)
{
	bool okWrite = _write(_GETDS, sizeof(_GETDS) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, "1.0") == 0)
	{
		*pParam = RNDS1_0;
	}
	else if (bcmp(_pRX, "0.5") == 0)
	{
		*pParam = RNDS0_5;
	}
	else if (bcmp(_pRX, "0.3") == 0)
	{
		*pParam = RNDS0_3;
	}
	else if (bcmp(_pRX, "none") == 0)
	{
		*pParam = RNDSNone;
	}
	else
	{
		return false;
	}

	return true;
}

bool RN2483::SetPreambleLength(unsigned int length)
{
	bool okWrite = _write(_SETPRLEN, sizeof(_SETPRLEN) - 1);
	if (!okWrite)
	{
		return false;
	}
	int i = sprintf(_pTX, "%u\r\n", length);
	if (i <= 0)
	{
		return false;
	}

	okWrite = _write(_pTX, i);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetPreambleLength(unsigned int *pLength)
{
	bool okWrite = _write(_GETPRLEN, sizeof(_GETPRLEN) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	unsigned int len;
	int i = sscanf(_pRX, "%u", &len);
	if (i != 1)
	{
		return false;
	}

	*pLength = len;

	return true;
}

bool RN2483::SetCRC(bool state)
{
	bool okWrite;
	if (state)
	{
		okWrite = _write(_SETCRCON, sizeof(_SETCRCON) - 1);
	}
	else
	{
		okWrite = _write(_SETCRCOFF, sizeof(_SETCRCOFF) - 1);
	}
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
	}

	return true;
}

bool RN2483::GetCRC(bool *pState)
{
	bool okWrite = _write(_GETCRC, sizeof(_GETCRC) - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, "on") == 0)
	{
		*pState = true;
		return true;
	}
	else if (bcmp(_pRX, "off") == 0)
	{
		*pState = false;
		return true;
	}

	return false;
}

bool RN2483::SetRXBW(RXBandWidth param)
{
	bool okWrite = _write(_SETRXBW, sizeof(_SETRXBW) - 1);
	if (!okWrite)
	{
		return false;
	}

	switch (param)
	{
		case RNRXBW250:
			_write("250\r\n", sizeof("250\r\n") - 1);
			break;

		case RNRXBW125:
			_write("125\r\n", sizeof("125\r\n") - 1);
			break;

		case RNRXBW62_5:
			_write("62.5\r\n", sizeof("62.5\r\n") - 1);
			break;

		case RNRXBW31_3:
			_write("31.3\r\n", sizeof("31.3\r\n") - 1);
			break;

		case RNRXBW15_6:
			_write("15.6\r\n", sizeof("15.6\r\n") - 1);
			break;

		case RNRXBW7_8:
			_write("7.8\r\n", sizeof("7.8\r\n") - 1);
			break;

		case RNRXBW3_9:
			_write("3.9\r\n", sizeof("3.9\r\n") - 1);
			break;

		case RNRXBW200:
			_write("200\r\n", sizeof("200\r\n") - 1);
			break;

		case RNRXBW100:
			_write("100\r\n", sizeof("100\r\n") - 1);
			break;

		case RNRXBW50:
			_write("50\r\n", sizeof("50\r\n") - 1);
			break;

		case RNRXBW25:
			_write("25\r\n", sizeof("25\r\n") - 1);
			break;

		case RNRXBW12_5:
			_write("12.5\r\n", sizeof("12.5\r\n") - 1);
			break;

		case RNRXBW6_3:
			_write("6.3\r\n", sizeof("6.3\r\n") - 1);
			break;

		case RNRXBW3_1:
			_write("3.1\r\n", sizeof("3.1\r\n") - 1);
			break;

		case RNRXBW166_7:
			_write("166.7\r\n", sizeof("166.7\r\n") - 1);
			break;

		case RNRXBW83_3:
			_write("83.3\r\n", sizeof("83.3\r\n") - 1);
			break;

		case RNRXBW41_7:
			_write("41.7\r\n", sizeof("41.7\r\n") - 1);
			break;

		case RNRXBW20_8:
			_write("20.8\r\n", sizeof("20.8\r\n") - 1);
			break;

		case RNRXBW10_4:
			_write("10.4\r\n", sizeof("10.4\r\n") - 1);
			break;

		case RNRXBW5_2:
			_write("5.2\r\n", sizeof("5.2\r\n") - 1);
			break;

		case RNRXBW2_6:
			_write("2.6\r\n", sizeof("2.6\r\n") - 1);
			break;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetRXBW(RXBandWidth *pParam)
{
	bool okWrite = _write(_GETRXBW, sizeof(_GETRXBW) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, "250") == 0)
	{
	        *pParam = RNRXBW250;
	}
	else if (bcmp(_pRX, "125") == 0)
	{
	        *pParam = RNRXBW125;
	}
	else if (bcmp(_pRX, "62.5") == 0)
	{
	        *pParam = RNRXBW62_5;
	}
	else if (bcmp(_pRX, "31.3") == 0)
	{
	        *pParam = RNRXBW31_3;
	}
	else if (bcmp(_pRX, "15.6") == 0)
	{
	        *pParam = RNRXBW15_6;
	}
	else if (bcmp(_pRX, "7.8") == 0)
	{
	        *pParam = RNRXBW7_8;
	}
	else if (bcmp(_pRX, "3.9") == 0)
	{
	        *pParam = RNRXBW3_9;
	}
	else if (bcmp(_pRX, "200") == 0)
	{
	        *pParam = RNRXBW200;
	}
	else if (bcmp(_pRX, "100") == 0)
	{
	        *pParam = RNRXBW100;
	}
	else if (bcmp(_pRX, "50") == 0)
	{
	        *pParam = RNRXBW50;
	}
	else if (bcmp(_pRX, "25") == 0)
	{
	        *pParam = RNRXBW25;
	}
	else if (bcmp(_pRX, "12.5") == 0)
	{
	        *pParam = RNRXBW12_5;
	}
	else if (bcmp(_pRX, "6.3") == 0)
	{
	        *pParam = RNRXBW6_3;
	}
	else if (bcmp(_pRX, "3.1") == 0)
	{
	        *pParam = RNRXBW3_1;
	}
	else if (bcmp(_pRX, "166.7") == 0)
	{
	        *pParam = RNRXBW166_7;
	}
	else if (bcmp(_pRX, "83.3") == 0)
	{
	        *pParam = RNRXBW83_3;
	}
	else if (bcmp(_pRX, "41.7") == 0)
	{
	        *pParam = RNRXBW41_7;
	}
	else if (bcmp(_pRX, "20.8") == 0)
	{
	        *pParam = RNRXBW20_8;
	}
	else if (bcmp(_pRX, "10.4") == 0)
	{
	        *pParam = RNRXBW10_4;
	}
	else if (bcmp(_pRX, "5.2") == 0)
	{
	        *pParam = RNRXBW5_2;
	}
	else if (bcmp(_pRX, "2.6") == 0)
	{
	        *pParam = RNRXBW2_6;
	}
	else
	{
		return false;
	}

	return true;
}

bool RN2483::SetWDT(unsigned int timeOut)
{
	bool okWrite = _write(_SETWDT, sizeof(_SETWDT) - 1);
	if (!okWrite)
	{
		return false;
	}
	int i = sprintf(_pTX, "%u\r\n", timeOut);
	if (i <= 0)
	{
		return false;
	}

	okWrite = _write(_pTX, i);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetWDT(unsigned int *pTimeOut)
{
	bool okWrite = _write(_GETWDT, sizeof(_GETWDT) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	unsigned int wdt;
	int i = sscanf(_pRX, "%u", &wdt);
	if (i != 1)
	{
		return false;
	}

	*pTimeOut = wdt;

	return true;
}

bool RN2483::SetSync(const char *pSync)
{
	bool okWrite = _write(_SETSYNC, sizeof(_SETSYNC) - 1);
	if (!okWrite)
	{
		return false;
	}

	okWrite = _write(pSync, strlen(pSync));
	if (!okWrite)
	{
		return false;
	}

	okWrite = _write("\r\n", sizeof("\r\n") - 1);
	if (!okWrite)
	{
		return false;
	}

	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (bcmp(_pRX, _OK) != 0)
	{
		return false;
	}

	return true;
}

bool RN2483::GetSync(char *pSync, size_t szMax)
{
	bool okWrite = _write(_GETWDT, sizeof(_GETWDT) - 1);
	if (!okWrite)
	{
		return false;
	}
	
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return false;
	}
	_pRX[szRead] = '\0';

	if (szRead - 2 > szMax - 1)
	{
		return false;
	}

	memcpy(pSync, _pRX, szRead - 2);
	pSync[szRead - 2] = '\0';

	return true;
}

bool RN2483::_write(const char *ptr, size_t sz)
{
	size_t szWrite = write(_fd, ptr, sz);
	if (szWrite == 0)
	{
		return false;
	}
	else if (szWrite != sz)
	{
		return false;
	}
	return true;
}

size_t RN2483::_read(char *ptr, size_t sz)
{
	return read(_fd, ptr, sz);
}
