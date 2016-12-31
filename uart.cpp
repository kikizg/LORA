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

	return true;
}

bool UART::TX(const void *ptr, size_t sz)
{
	// translate data
	size_t szHex = D2H(static_cast<const char*>(ptr), sz, reinterpret_cast<char*>(_pTX));

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
	size_t szHex = D2H(static_cast<const char*>(ptr1), sz1, reinterpret_cast<char*>(_pTX));

	bool okWrite = _write(_pTX, szHex);
	if (!okWrite)
	{
		return false;
	}

	// translate and send second data segment
	szHex = D2H(static_cast<const char*>(ptr2), sz2, reinterpret_cast<char*>(_pTX));
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
	size_t szHex = H2D(reinterpret_cast<const char*>(_pRX), szRead - 1, static_cast<char*>(pDst), szDst);

	return szHex;
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
