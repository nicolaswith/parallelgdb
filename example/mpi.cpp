#include <iostream>
#include <string>
#include <string.h>
#include <mpi.h>

using namespace std;

#define SEND_LENGTH 100

int main(int argc, char const **argv)
{
	MPI_Init(NULL, NULL);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	cout << "Rank: " << rank << ", Size: " << size << "\n";

	string send_str = string("Hello from Rank ") + to_string(rank) + ".\n";

	if (0 == rank)
	{
		char **rec_strs = new char *[size];
		rec_strs[0] = strdup(send_str.c_str());

		for (int i = 1; i < size; ++i)
		{
			MPI_Status status;
			char *tmp_rec = new char[SEND_LENGTH]();

			MPI_Recv(tmp_rec, SEND_LENGTH, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			rec_strs[status.MPI_SOURCE] = strdup(tmp_rec);

			delete[] tmp_rec;
		}

		cout << "\n";
		for (int i = 0; i < size; ++i)
		{
			cout << rec_strs[i];
			free(rec_strs[i]);
		}

		delete[] rec_strs;
	}
	else
	{
		MPI_Send(send_str.c_str(), send_str.length(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	return 0;
}