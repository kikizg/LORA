#include "comm.h"

using namespace std;
using namespace RN;

int main(int argc, char **ppArgv, char **ppEnv)
{
	if (argc != 2)
	{
		return false;
	}

	char data[1024];
	for (int i = 0; i < sizeof(data); i++)
	{
		data[i] = i;
	}

	if (*ppArgv[1] == 's')
	{

		Comm c;
		c.Init();

		PacketInfo info;
		info.FromId = 0xaa;
		info.ToId = 0xcc;
		info.Port = 0x20;

		c.SetInfo(&info);
		printf("TX: %i\n", c.SendAck(data, sizeof(data)));
	}
	else
	{
		Comm c;
		c.Init();

		PacketInfo info;
		info.FromId = 0xaa;
		info.ToId = 0xcc;
		info.Port = 0x20;

		c.SetInfo(&info);
		printf("RX: %i\n", c.Receive(data, sizeof(data)));

		for (int i = 0; i < sizeof(data); i++)
		{
			char res = i;
			if (data[i] != res) 
			{
				printf("Error: Data %i instead %i\r\n", data[i], res);
			}
		}
	}


}
