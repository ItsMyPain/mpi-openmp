#include <mpi.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int world_rank, world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int number, counter = 0;
    int N = 1e6;

    if (world_rank == 0) {
        clock_t begin = clock();

        while (counter < N) {
            MPI_Ssend(&number, 1, MPI_INT, 5, 0, MPI_COMM_WORLD);
            MPI_Recv(&number, 1, MPI_INT, 5, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            counter += 1;
        }

        clock_t end = clock();
        printf("The elapsed time is %f seconds", (double)(end - begin) / CLOCKS_PER_SEC);

    } else if (world_rank == 5) {

        while (counter < N) {
            MPI_Recv(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Ssend(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            counter += 1;
        }
    }


    MPI_Finalize();
    return 0;
}