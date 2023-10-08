#include <mpi.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int world_rank, world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    const size = 1e6;
    static int data[(int)1e6] = {0};
    int N = 1e4;
    int counter = 0;

    if (world_rank == 0) {
        clock_t begin = clock();

        while (counter < N) {
            MPI_Send(data, size, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(data, size, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            counter += 1;
        }

        clock_t end = clock();
        printf("The elapsed time is %f seconds", (double)(end - begin) / CLOCKS_PER_SEC);
	printf("%zu", sizeof(int));

    } else if (world_rank == 1) {

        while (counter < N) {
            MPI_Recv(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
            counter += 1;
        }
    }


    MPI_Finalize();
    return 0;
}