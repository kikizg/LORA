#include <boost/program_options.hpp>
#include <vector>
#include <string>
#include <ctime>
#include <fstream>
#include <iostream>
#include<openssl/rsa.h>
#include<openssl/pem.h>

#define _DEBUG

#include "comm.h"
#include "clock.h"

#define WHITE "\033[0m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BROWN "\033[1;33m"

using namespace std;
using namespace RN;
namespace po = boost::program_options;

void parse_args(int argc, char **argv, po::options_description &optDesc, po::variables_map &varMap);
bool generate_key(const char *pPublicPath, const char *pPrivatePath, int bits);

int main(int argc, char **argv, char **envp)
{
	po::options_description desc;
	po::variables_map vm;
	parse_args(argc, argv, desc, vm); 

	if (vm.count("genkey"))
	{
		bool ok = generate_key(
			vm["publickey"].as<string>().data(),
			vm["privatekey"].as<string>().data(),
			vm["keybits"].as<int>());

		if (!ok)
		{
			return -1;
		}
	}

	if (vm.count("transmit"))
 	{
		Comm c;
 		c.Init();
 
 		PacketInfo info;
 		info.LocalId = 0xaa;
 		info.RemoteId = 0xcc;
 		info.Port = 0x20;

 		c.SetInfo(&info);
		c.SetCrypt("key.pub", "key");

		ifstream ifs;

		size_t total = 0;

		if (vm.count("input"))
		{
			ifs.open(vm["input"].as<string>().data(), fstream::in | fstream::binary);

			ifs.seekg(0, ios_base::end);
			total = ifs.tellg();
			ifs.seekg(0, ios_base::beg);

		}
		
		istream &is = vm.count("input") ? ifs : cin;

		size_t szBuf = c.GetSzDecryptBuf();
		size_t szData = szBuf - 1;
		char *pBuf = new char[szBuf];
		char *pData = pBuf + 1;

		*pBuf = is.good();

		size_t sent = 0;


#ifdef _DEBUG
		if (total)
		{
 			printf(GREEN "[OK]" WHITE " DATA SEND START size(%u)\n", total);
		}
		else
		{
 			cout << GREEN "[OK]" WHITE " DATA SEND START\n";
		}
#endif

		bool okSend;
		while (*pBuf)
		{
			is.read(pData, szData);
			*pBuf = is.good();
			size_t read = is.gcount();

			// okSend = c.Send(pBuf, read + 1, true);
			okSend = c.EncryptPubSend(pBuf, read + 1, true);
			// okSend = c.EncryptPvtSend(pBuf, read + 1, true);
			if (!okSend)
			{
#ifdef _DEBUG
				if (total)
				{
					printf(RED "[ERROR]" WHITE " DATA SEND size(%u/%u)\n", read, total);
				}
				else
				{
					printf(RED "[ERROR]" WHITE " DATA SEND size(%u)\n", read);
				}
#endif
				break;
			}

#ifdef _DEBUG
			if (total)
			{
 				printf(GREEN "[OK]" WHITE " DATA SEND size(%u/%u)\n", read, total);
			}
			else
			{
 				printf(GREEN "[OK]" WHITE " DATA SEND size(%u)\n", read);
			}
#endif

			sent += read;
		}

#ifdef _DEBUG
		if (okSend)
		{
			cout << GREEN "[OK]" WHITE;
		}
		else
		{
			cout << RED "[ERROR]" WHITE;
		}

		if (total)
		{
			printf(" DATA SEND END size(%u/%u)\n", sent, total);
		}
		else
		{
			printf(" DATA SEND END size(%u)\n", sent);
		}
#endif

		delete[] pBuf;
 	}
	else if (vm.count("receive"))
 	{
		Comm c;
 		c.Init();
 
 		PacketInfo info;
 		info.LocalId = 0xcc;
 		info.RemoteId = 0xaa;
 		info.Port = 0x20;

 		c.SetInfo(&info);
		c.SetCrypt("key.pub", "key");

		size_t szBuf = c.GetSzDecryptBuf();
		size_t szData = szBuf - 1;
		char *pBuf = new char[szBuf];
		char *pData = pBuf + 1;
		
		ofstream ofs;

		if (vm.count("output"))
		{
			ofs.open(vm["output"].as<string>().data(), fstream::out | fstream::binary | fstream::trunc);
		}
		
		ostream &os = vm.count("output") ? ofs : cout;

#ifdef _DEBUG
 		cout << GREEN "[OK]" WHITE " DATA RECEIVE START\n";
#endif

		size_t received = 0;
		bool okRX;

		do
		{
			size_t szRX;
			okRX = c.ReceiveDecryptPvt(pBuf, szBuf, &szRX);
			// okRX = c.ReceiveDecryptPub(pBuf, szBuf, &szRX);
			// okRX = c.Receive(pBuf, szBuf, &szRX);
			if (!okRX || szRX < 2)
			{
#ifdef _DEBUG
					printf(RED "[ERROR]" WHITE " DATA RECEIVE size(%u)\n", szRX ? szRX - 1 : 0);
#endif
				break;
			}

#ifdef _DEBUG
 			printf(GREEN "[OK]" WHITE " DATA RECEIVE size(%u)\n", szRX - 1);
#endif

			os.write(pData, (szRX < szBuf ? szRX : szBuf) - 1);
			os.flush();
			received += szRX - 1;
		} while(*pBuf);

#ifdef _DEBUG
		if (okRX)
		{
			cout << GREEN "[OK]" WHITE;
		}
		else
		{
			cout << RED "[ERROR]" WHITE;
		}
		printf(" DATA RECEIVE END size(%u)\n", received);
#endif

		delete[] pBuf;
 	}
	else
	{
		// show help if no valid parameters is set

		cout << desc << "\n";
	}

	return 0;
};

bool generate_key(const char *pPublicPath, const char *pPrivatePath, int bits)
{
	double time = Clock::Total();
	RAND_seed(&time, sizeof(double));

	unsigned long type = RSA_F4;
	BIGNUM *pBN = BN_new();

	if (!pBN)
	{
		return false;
	}

	BN_set_word(pBN, type);

	RSA *pRSA = RSA_new();

	if (!pRSA)
	{
		return false;
	}

	int status = RSA_generate_key_ex(pRSA, bits, pBN, NULL);

	if (!status)
	{
		return false;
	}
	
	BIO *pPublic = BIO_new_file(pPublicPath, "w+");
	BIO *pPrivate = BIO_new_file(pPrivatePath, "w+");

	if (!(pPublic && pPrivate))
	{
		return false;
	}

	status = PEM_write_bio_RSAPublicKey(pPublic, pRSA);

	if (!status)
	{
		return false;
	}

	status = PEM_write_bio_RSAPrivateKey(pPrivate, pRSA, NULL, NULL, 0, NULL, NULL);

	if(!status)
	{
		return false;
	}

	BIO_free_all(pPublic);
	BIO_free_all(pPrivate);

	BN_free(pBN);
	RSA_free(pRSA);

	return true;
};

void parse_args(int argc, char **argv, po::options_description &optDesc, po::variables_map &varMap)
{
	optDesc.add_options()
		("help,h", "Help screen")
		("transmit,t", "Transmit data from TX buffer")
		("receive,r", "Receive data into RX buffer")
		("publickey,k", po::value<string>(), "Public key to use with RSA encryption")
		("privatekey,i", po::value<string>(), "Private key to use with RSA encryption")
		("genkey,g", "Generate public and private RSA key")
		("keybits,b", po::value<int>(), "Key size [bit]")
		("input,f", po::value<string>(), "Input file name which will be sent")
		("output,o", po::value<string>(), "Output file name into which received data will be stored")
		("encryptpub", "Encrypt data with public key")
		("encryptpvt", "Encrypt data with private key")
		("decryptpub", "Decrypt data with public key")
		("decryptpvt", "Decrypt data with private key");

	po::store(po::parse_command_line(argc, argv, optDesc), varMap);
	po::notify(varMap);
};
