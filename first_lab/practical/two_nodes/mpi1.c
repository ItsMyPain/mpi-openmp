#include <mpi.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int world_rank, world_size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int N = 1e4;
    int M = 21;
    for (int i = 0; i < M; i++) {
        int counter = 0;
        int size = 1;
        for (int j = 0; j < i; j++) {
            size *= 2;
        }
        int* data = malloc(size * sizeof(int));

        if (world_rank == 0) {
            clock_t begin = clock();

            while (counter < N) {
                MPI_Send(data, size, MPI_INT, 5, 0, MPI_COMM_WORLD);
                MPI_Recv(data, size, MPI_INT, 5, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                counter += 1;
            }

            clock_t end = clock();
            printf("%f, ", (double)(end - begin) / CLOCKS_PER_SEC);

        } else if (world_rank == 5) {

            while (counter < N) {
                MPI_Recv(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(data, size, MPI_INT, 0, 0, MPI_COMM_WORLD);
                counter += 1;
            }
        }
    }


    MPI_Finalize();
    return 0;
}