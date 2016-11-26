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
	// Modulation types for TX/RX on device.
	enum Mod : unsigned char
	{
		RNLORA,		// LORA modulation.
		RNFSK		// FSK modulation.
	};

	// Data shaping parameter for FSK modulation.
	enum DataShaping : unsigned char
	{
		RNDS1_0,	// Data shaping 1.0.
		RNDS0_5,	// Data shaping 0.5.
		RNDS0_3,	// Data shaping 0.5.
		RNDSNone	// None data shaping.
	};

	// Set radio bandwidth for RX.
	enum RXBandWidth : unsigned char
	{
		RNRXBW250,	// 250 [kHz]
		RNRXBW125,	// 125 [kHz]
		RNRXBW62_5,	// 62.5 [kHz]
		RNRXBW31_3,	// 31.3 [kHz]
		RNRXBW15_6,	// 15.6 [kHz]
		RNRXBW7_8,	// 7.8 [kHz]
		RNRXBW3_9,	// 3.9 [kHz]
		RNRXBW200,	// 200 [kHz]
		RNRXBW100,	// 100 [kHz]
		RNRXBW50,	// 50 [kHz]
		RNRXBW25,	// 25 [kHz]
		RNRXBW12_5,	// 12.5 [kHz]
		RNRXBW6_3,	// 6.3 [kHz]
		RNRXBW3_1,	// 3.1 [kHz]
		RNRXBW166_7,	// 166.7 [kHz]
		RNRXBW83_3,	// 83.3 [kHz]
		RNRXBW41_7,	// 41.7 [kHz]
		RNRXBW20_8,	// 20.8 [kHz]
		RNRXBW10_4,	// 10.4 [kHz]
		RNRXBW5_2,	// 5.2 [kHz]
		RNRXBW2_6	// 2.6 [kHz]
	};

	// Class used for communication with RN2483 device throuh Comm.
	class RN2483
	{
		public:
			// Default class constructor which opens Comm stream.
			RN2483();

			// Destrcutor for class.
			// Comm stream and release all resources.
			~RN2483();

			// Initialize RN2483 device and get ready for TX/RX.
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

			// Send mac pause command to pause LORAWAN stack.
			// Returns time [milisecond] on success, 0 on failure.
			unsigned int SetMACPause();

			// Send mac resume command to resume LORAWAN stack.
			// Returns true on success, false on failure.
			bool SetMACResume();

			// Reset device (send sys reset command).
			// Returns true on success, false on failure.
			bool Reset();

			// Set modulation for RX/TX on device.
			// Returns true on success, false on failure.
			bool SetMod(Mod modulation);

			// Get modulaion for RX/TX on device.
			// pModulation: Pointer where type of modulation will be stored.
			// Returns true on success, false on failure.
			bool GetMod(Mod *pModulation);

			// Set modulation frequency for RX/TX on device.
			// freq: Frequency in interval 433050000-434790000 for FSK or
			// 863000000-870000000 for LORA [Hz].
			// Returns true on success, false on failure.
			bool SetFreq(unsigned int freq);

			// Get modulation frequency for RX/TX on device.
			// pFreq: Pointer where frequency will be stored.
			// Returns true on success, false on failure.
			bool GetFreq(unsigned int *pFreq);

			// Set power of RX/TX on device.
			// power: Power from -3 to 15.
			// Returns true on success, false on failure.
			bool SetPower(char power);

			// Get power of RX/TX on device.
			// pPower: Pointer where number representing power will be stored.
			// Returns true on success, false on failure.
			bool GetPower(char *pPower);

			// Set bit rate for FSK modulation on device.
			// rate: Bit rate 1 - 300000 [bps].
			// Returns true on success, false on failure.
			bool SetBitRate(unsigned int rate);

			// Get bit rate for FSK modulation on device.
			// pRate: Pointer where bit rate [bps] will be stored.
			// Returns true on success, false on failure.
			bool GetBitRate(unsigned int *pRate);

			// Set FSK data shaping parameter.
			// param: Data shaping parameter used in FSK modulation.
			// Returns true on success, false on failure.
			bool SetDataShaping(DataShaping param);

			// Get FSK data shaping parameter.
			// pParam: Pointer where data shaping parameter will be stored.
			// Returns true on success, false on failure.
			bool GetDataShaping(DataShaping *pParam);

			// Set preamble length used for TX/RX.
			// length: Length of preamble.
			// Returns true on success, false on failure.
			bool SetPreambleLength(unsigned int length);

			// Get preamble length used for TX/RX.
			// pLength: Pointer where preamble length will be stored.
			// Returns true on success, false on failure.
			bool GetPreambleLength(unsigned int *pLength);

			// Set state of CFC used for TX/RX.
			// state: State of CRC.
			// Returns true on success, false on failure.
			bool SetCRC(bool state);

			// Get state of CRC used for TX/RX.
			// pLength: Pointer where CRC state will be stored.
			// Returns true on success, false on failure.
			bool GetCRC(bool *pState);

			// Set radio bandwidth for RX.
			// param: Bandwidth parameter.
			// Returns true on success, false on failure.
			bool SetRXBW(RXBandWidth param);

			// Get radio bandwidth for RX.
			// pParam: Pointer where radio bandwidth parameter will be stored.
			// Returns true on success, false on failure.
			bool GetRXBW(RXBandWidth *pParam);

			// Set watch dog timeout.
			// timeOut: Watch dog timeout parameter.
			// Returns true on success, false on failure.
			bool SetWDT(unsigned int timeOut);

			// Get watch dog timeout.
			// pTimeOut: Pointer where WDT value will be stored.
			// Returns true on success, false on failure.
			bool GetWDT(unsigned int *pTimeOut);

			// Set sync word for RX/TX.
			// pSync: Pointer to null terminated string which will be sync word (null character is excluded).
			// Returns true on success, false on failure.
			bool SetSync(const char *pSync);

			// Get sync word for RX/TX.
			// pSync: Pointer where sync word will be stored (terminated with null character).
			// szMax: Max size of buffer pSync (including null terminated character). 
			// Returns true on success, false on failure.
			bool GetSync(char *pSync, size_t szMax);

		private:
			// Function send data through UART port.
			// ptr: Pointer to data which will be send.
			// sz: Size of data which will be send.
			// Returns true on success, false on failure.
			bool _write(const char *ptr, size_t sz);

			// Function read data through UART port.
			// ptr: Pointer to data which will be read.
			// sz: Size of data which will be read.
			// Returns size of received data [bytes] or -1 on failure.
			size_t _read(char *ptr, size_t sz);

			// Translate decimal data into hex data and store it inside _pBuf.
			// pIn: Pointer to input buffer which will be translated.
			// szIn: Size of data which needs to be translated [bytes].
			// Returns size of translated data [bytes].
			size_t _d2hex(const char *pIn, size_t szIn);

			// Translate hex data into decimal data and store it inside _pBuf.
			// pIn: Pointer to input buffer which will be translated.
			// szIn: Size of data which needs to be translated [bytes].
			// pOut: Pointer to output buffer where translated data will be stored.
			// szOut: Size of buffer where translated data will be stored [bytes].
			// Returns size of translated data [bytes].
			size_t _hex2d(const char *pIn, size_t szIn, char *pOut, size_t szOut);

			static const
			size_t _szBuf;		// Max size of frame (bytes to be send) [bytes].

			int _fd;		// File descriptor for Comm stream.

			char *_pTX;		// Temporary internal TX buffer.
			char *_pRX;		// Temporary internal RX buffer.

			// Commands used for internal communication with RN2483 device.
			static const char _DNULL[];
			static const char _UVER[];
			static const char _MACPAUSE[];
			static const char _MACRESUME[];
			static const char _OK[];
			static const char _TXS[];
			static const char _TXOK[];
			static const char _RX[];
			static const char _RXR[];
			static const char _RXE[];
			static const char _RESET[];
			static const char _RESET_ACK[];
			static const char _END[];
			static const char _GETMOD[];
			static const char _GETMODFSK[];
			static const char _GETMODLORA[];
			static const char _SETMODFSK[];
			static const char _SETMODLORA[];
			static const char _SETFREQ[];
			static const char _GETFREQ[];
			static const char _SETPWR[];
			static const char _GETPWR[];
			static const char _SETRATE[];
			static const char _GETRATE[]; 
			static const char _SETDS[];
			static const char _GETDS[];
			static const char _SETPRLEN[];
			static const char _GETPRLEN[];
			static const char _SETCRCON[];
			static const char _SETCRCOFF[];
			static const char _GETCRC[];
			static const char _SETRXBW[];
			static const char _GETRXBW[];
			static const char _SETWDT[];
			static const char _GETWDT[];
			static const char _SETSYNC[];
			static const char _GETSYNC[];
			static const char _INVLD[];
	};
}
