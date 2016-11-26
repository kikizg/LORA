#include "uart.h"
#include <string>

using namespace std;
using namespace RN;

const size_t UART::_szBuf = 1024;

UART::UART() :
	_fd(-1),
	_pTX(new unsigned char[_szBuf + 1]),
	_pRX(new unsigned char[_szBuf + 1])
{
}

UART::~UART()
{
	close(_fd);
	delete[] _pTX;
	delete[] _pRX;
}

bool UART::Init()
{
	_fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY);
	if (_fd == -1)
	{
		return false;
	}
	
	int rc;
	struct termios options;
	memset(&options, 0, sizeof(termios));
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_cflag = CS8 | CLOCAL | CREAD;
	options.c_lflag = ICANON;
	options.c_cc[VEOL] = '\n';

	rc = cfsetspeed(&options, B115200);
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

	return true;
}

bool UART::TX(const void *ptr, size_t sz)
{
	// translate data
	size_t szHex = _d2hex(static_cast<const unsigned char*>(ptr), sz);

	// transmit data
	bool okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	const unsigned char end[] = "\n";
	okWrite = _write(end, sizeof(end) - 1);
	if (!okWrite)
	{
		return false;
	}

	return true;
}

bool UART::TX(const void *ptr1, size_t sz1, const void *ptr2, size_t sz2)
{
	// translate and send first data segment
	size_t szHex = _d2hex(static_cast<const unsigned char*>(ptr1), sz1);

	bool okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	// translate and send second data segment
	szHex = _d2hex(static_cast<const unsigned char*>(ptr2), sz2);
	okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	const unsigned char end[] = "\n";
	okWrite = _write(end, sizeof(end) - 1);
	if (!okWrite)
	{
		return false;
	}

	return true;
}

size_t UART::RX(void *pDst, size_t szDst)
{
	size_t szRead = _read(_pRX, _szBuf);
	if (szRead == 0)
	{
		return 0;
	}
	_pRX[szRead] = '\0';

	// if data is received
	size_t szHex = _hex2d(_pRX, szRead - 1, static_cast<unsigned char*>(pDst), szDst);

	return szHex;
}

size_t UART::_d2hex(const unsigned char *pIn, size_t szIn)
{
	if (szIn * 2 > _szBuf)
	{
		return 0;
	}

	short *ptr = reinterpret_cast<short*>(_pTX);
	for (size_t i = 0; i < szIn; i++)
	{
		ptr[i] = RN_D2HEX[pIn[i]];
	}

	return szIn * 2;
}

size_t UART::_hex2d(const unsigned char *pIn, size_t szIn, unsigned char *pOut, size_t szOut)
{
	if (szIn / 2 + szIn % 2 > szOut)
	{
		return 0;
	}

	for (size_t i = 0; i < szIn; i += 2)
	{
		pOut[i / 2] = RN_HEX2D[pIn[i]] << 4 | RN_HEX2D[pIn[i + 1]];
	}

	return szIn / 2 + szIn % 2;
}

bool UART::_write(const unsigned char *ptr, size_t sz)
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

size_t UART::_read(unsigned char *ptr, size_t sz)
{
	size_t szRead = read(_fd, ptr, sz);
	if (szRead == 0)
	{
		return szRead;
	}

	return szRead;
}
