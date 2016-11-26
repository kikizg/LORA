#pragma once

#include <iostream>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "translation.h"

using namespace std;

namespace RN
{
	// Class used for communication with UART device throuh Comm.
	class UART
	{
		public:
			// Default class constructor which opens Comm stream.
			UART();

			// Destrcutor for class.
			// Comm stream and release all resources.
			~UART();

			// Initialize UART device and get ready for TX/RX.
			// Returns true on success or false on failure.
			bool Init();

			// Send data through Comm.
			// ptr: Pointer to data.
			// sz: Size of data [bytes].
			// Returns true on success of false on failure.
			bool TX(const void *ptr, size_t sz);

			// Send data through Comm.
			// ptr1: Pointer to first segment of TX data.
			// sz1: Size of first data segment [byte].
			// ptr2: Pointer to second segment of TX data.
			// sz2: Size of second data segment [byte].
			// Returns true on success of false on failure.
			bool TX(const void *ptr1, size_t sz1, const void *ptr2, size_t sz2);


			// Receive data through Comm. Data stream is captured until CR LF characters are received.
			// ptr: Pointer to buffer where received data will be stored as null terminated string without CR LF at the end.
			// sz: Size of pDst buffer. If size of received data is larger than szDst, data will not be stored at pDst.
			// Returns number of data successfully read [bytes] or 0 on failure.
			size_t RX(void *ptr, size_t sz);

		private:
			// Function send data through UART port.
			// ptr: Pointer to data which will be send.
			// sz: Size of data which will be send.
			// Returns true on success, false on failure.
			bool _write(const unsigned char *ptr, size_t sz);

			// Function read data through UART port.
			// ptr: Pointer to data which will be read.
			// sz: Size of data which will be read.
			// Returns size of received data [bytes] or 0 on failure.
			size_t _read(unsigned char *ptr, size_t sz);

			// Translate decimal data into hex data and store it inside _pBuf.
			// pIn: Pointer to input buffer which will be translated.
			// szIn: Size of data which needs to be translated [bytes].
			// Returns size of translated data [bytes].
			size_t _d2hex(const unsigned char *pIn, size_t szIn);

			// Translate hex data into decimal data and store it inside _pBuf.
			// pIn: Pointer to input buffer which will be translated.
			// szIn: Size of data which needs to be translated [bytes].
			// pOut: Pointer to output buffer where translated data will be stored.
			// szOut: Size of buffer where translated data will be stored [bytes].
			// Returns size of translated data [bytes].
			size_t _hex2d(const unsigned char *pIn, size_t szIn, unsigned char *pOut, size_t szOut);

			static const
			size_t _szBuf;		// Max size of frame (bytes to be send) [bytes].

			int _fd;		// File descriptor for Comm stream.

			unsigned char *_pTX;	// Temporary internal TX buffer.
			unsigned char *_pRX;	// Temporary internal RX buffer.
	};
}
