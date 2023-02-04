#include <string>
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>

using std::string;

int getRank()
{
	const char *rank_cstr = getenv("PMI_RANK");
	if (!rank_cstr)
	{
		rank_cstr = getenv("OMPI_COMM_WORLD_RANK");
	}
	if (!rank_cstr)
	{
		return -1;
	}
	return std::stoi(string(rank_cstr));
}

int main(int argc, char const *argv[])
{
	int rank = getRank();

	printf("\n\n ### Start Test Program %d ### \n\n\n", rank);

	int val = 0;
	printf("val = %d\n", val);

	bool state = true;
	while (state)
	{
		int tmp = 1;
		val += tmp;
	}

	printf("\n\n ### End Test Program ### \n\n\n");

	if (rank == 1)
	{
		int ret = 1;
		return ret;
	}

	return 0;
}