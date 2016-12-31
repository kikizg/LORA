#include <cstdio>

#include "clock.h"
#include "rn2483.h"

#define MSGLEN 20
#define ITER 20

using namespace RN;

int main()
{
	char txt[MSGLEN];
	for (size_t idx = 0; idx < MSGLEN; idx++)
	{
		txt[idx] = idx + 1;
	}

	RN2483 rn;
	rn.Init();

	Clock clk;

	for (unsigned int i = 0; i < ITER; i++)
	{
		rn.TX(txt, sizeof(txt));
	}

	double elapsed = clk.Now();
	printf("TX total %i bytes by %i bytes each group which took %f seconds with bandwidth %i B/s\n", ITER * MSGLEN, MSGLEN, elapsed, static_cast<int>(static_cast<double>(ITER * MSGLEN) / elapsed));
};
