/*
 * Author: Nikolay Khokhlov <k_h@inbox.ru>, 2016
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <mpi.h>

#define ind(i, j) (((i + l->nx) % l->nx) + ((j + l->ny) % l->ny) * (l->nx))

typedef struct {
	int nx, ny;
	int *u0;
	int *u1;
	int steps;
	int save_steps;

	/* Boundaries. */
	int start[2];
	int end[2];

	/* Cartesian communicator. */
	MPI_Comm comm_cart;
	MPI_Comm comm_dims[2];
	int dims[2];
	int coords[2];
	MPI_Datatype block_t;
	MPI_Datatype rows_t;

	MPI_Datatype type_col;
	MPI_Datatype type_row;

	/* Debug. */
	int rank;
	int size;
} life_t;

void life_init(const char *path, life_t *l);
void life_free(life_t *l);
void life_step(life_t *l);
void life_save_vtk(const char *path, life_t *l);
void life_decomposition(int n, int size, int rank, int *start, int *end);
void life_gather_1d(life_t *l, MPI_Comm comm, MPI_Datatype block_t, int ind_send, int ind_recv);
void life_gather(life_t *l);

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s <input file> <output file>.\n", argv[0]);
		return 0;
	}
	MPI_Init(&argc, &argv);
	life_t l;
	life_init(argv[1], &l);

    double start_time;
    double end_time;

    if (l.rank == 0) {
		start_time = MPI_Wtime();
	}

	int i;
	char buf[100];
	for (i = 0; i < l.steps; i++) {
//		if (i % l.save_steps == 0) {
//			life_gather(&l);
//			if (l.rank == l.size - 1) {
//				sprintf(buf, "result/life_%06d.vtk", i);
//				printf("Saving step %d to '%s'.\n", i, buf);
//				life_save_vtk(buf, &l);
//			}
//		}
		life_step(&l);
	}

    if (l.rank == 0) {
		end_time = MPI_Wtime();
		printf("%d, time: %f \n", l.size, end_time - start_time);

		FILE* f = fopen(argv[2], "a");
		fprintf(f, "%d %f \n", l.size, end_time - start_time);
		fclose(f);
	}

	life_free(&l);
	MPI_Finalize();
	return 0;
}

/**
 * Загрузить входную конфигурацию.
 * Формат файла, число шагов, как часто сохранять, размер поля, затем идут координаты заполненых клеток:
 * steps
 * save_steps
 * nx ny
 * i1 j2
 * i2 j2
 */
void life_init(const char *path, life_t *l)
{
	FILE *fd = fopen(path, "r");
	assert(fd);
	assert(fscanf(fd, "%d\n", &l->steps));
	assert(fscanf(fd, "%d\n", &l->save_steps));
	printf("Steps %d, save every %d step.\n", l->steps, l->save_steps);
	assert(fscanf(fd, "%d %d\n", &l->nx, &l->ny));
	printf("Field size: %dx%d\n", l->nx, l->ny);

	l->u0 = (int*)calloc(l->nx * l->ny, sizeof(int));
	l->u1 = (int*)calloc(l->nx * l->ny, sizeof(int));

	int i, j, r, cnt;
	cnt = 0;
	while ((r = fscanf(fd, "%d %d\n", &i, &j)) != EOF) {
		l->u0[ind(i, j)] = 1;
		cnt++;
	}
	printf("Loaded %d life cells.\n", cnt);
	fclose(fd);

	/* Decomposition. */
	MPI_Comm_rank(MPI_COMM_WORLD, &(l->rank));
	MPI_Comm_size(MPI_COMM_WORLD, &(l->size));
	l->dims[0] = l->dims[1] = 0;
	MPI_Dims_create(l->size, 2, l->dims);
	//printf("#%d: dims = {%d, %d}\n", l->rank, l->dims[0], l->dims[1]);
	int periods[2] = {1, 1};
	MPI_Cart_create(MPI_COMM_WORLD, 2, l->dims, periods, 0, &(l->comm_cart));
	MPI_Cart_coords(l->comm_cart, l->rank, 2, l->coords);
	//printf("#%d: coords = {%d, %d}\n", l->rank, l->coords[0], l->coords[1]);
	life_decomposition(l->nx, l->dims[0], l->coords[0], l->start, l->end);
	life_decomposition(l->ny, l->dims[1], l->coords[1], l->start+1, l->end+1);
	//printf("#%d: start = {%d, %d}\n", l->rank, l->start[0], l->start[1]);
	//printf("#%d: end = {%d, %d}\n", l->rank, l->end[0], l->end[1]);


	/* Row/col communicators. */
	MPI_Comm_split(l->comm_cart, l->coords[1], l->coords[0], l->comm_dims);
	MPI_Comm_split(l->comm_cart, l->coords[0], l->coords[1], l->comm_dims + 1);

	int start[2], end[2];
	life_decomposition(l->nx, l->dims[0], 0, start, end);
	life_decomposition(l->ny, l->dims[1], l->coords[1], start+1, end+1);
	MPI_Datatype temp;
	MPI_Type_vector(end[1] - start[1], end[0] - start[0], l->nx, MPI_INT, &temp);
	MPI_Type_create_resized(temp, 0, (end[0] - start[0]) * sizeof(int), &(l->block_t));
	MPI_Type_commit(&(l->block_t));

	life_decomposition(l->ny, l->dims[1], 0, start+1, end+1);
	MPI_Type_contiguous(l->nx * (end[1] - start[1]), MPI_INT, &(l->rows_t));
	MPI_Type_commit(&(l->rows_t));

    MPI_Type_vector(1, l->end[0] - l->start[0], 0, MPI_INT, &(l->type_row));
	MPI_Type_commit(&(l->type_row));

    MPI_Type_vector(l->end[1] - l->start[1], 1, l->nx, MPI_INT, &(l->type_col));
	MPI_Type_commit(&(l->type_col));
}

void life_free(life_t *l)
{
	free(l->u0);
	free(l->u1);
	l->nx = l->ny = 0;

	MPI_Type_free(&(l->block_t));
    MPI_Type_free(&(l->rows_t));

    MPI_Type_free(&(l->type_col));
	MPI_Type_free(&(l->type_row));

    MPI_Comm_free(&(l->comm_cart));
    MPI_Comm_free(&(l->comm_dims[0]));
    MPI_Comm_free(&(l->comm_dims[1]));
}

void life_save_vtk(const char *path, life_t *l)
{
	FILE *f;
	int i1, i2, j;
	f = fopen(path, "w");
	assert(f);
	fprintf(f, "# vtk DataFile Version 3.0\n");
	fprintf(f, "Created by write_to_vtk2d\n");
	fprintf(f, "ASCII\n");
	fprintf(f, "DATASET STRUCTURED_POINTS\n");
	fprintf(f, "DIMENSIONS %d %d 1\n", l->nx+1, l->ny+1);
	fprintf(f, "SPACING %d %d 0.0\n", 1, 1);
	fprintf(f, "ORIGIN %d %d 0.0\n", 0, 0);
	fprintf(f, "CELL_DATA %d\n", l->nx * l->ny);

	fprintf(f, "SCALARS life int 1\n");
	fprintf(f, "LOOKUP_TABLE life_table\n");
	for (i2 = 0; i2 < l->ny; i2++) {
		for (i1 = 0; i1 < l->nx; i1++) {
			fprintf(f, "%d\n", l->u0[ind(i1, i2)]);
		}
	}
	fclose(f);
}

void life_step(life_t *l)
{
	int i, j;

    MPI_Send(l->u0 + ind(l->end[0] - 1, l->start[1]), 1, l->type_col, (l->coords[0] + 1) % l->dims[0], 0, l->comm_dims[0]);
	MPI_Recv(l->u0 + ind(l->start[0] - 1, l->start[1]), 1, l->type_col, (l->coords[0] - 1 + l->dims[0]) % l->dims[0], 0, l->comm_dims[0], MPI_STATUS_IGNORE);
	MPI_Send(l->u0 + ind(l->start[0], l->start[1]), 1, l->type_col, (l->coords[0] - 1 + l->dims[0]) % l->dims[0], 0, l->comm_dims[0]);
	MPI_Recv(l->u0 + ind(l->end[0], l->start[1]), 1, l->type_col, (l->coords[0] + 1) % l->dims[0], 0, l->comm_dims[0], MPI_STATUS_IGNORE);

    MPI_Send(l->u0 + ind(l->start[0], l->end[1] - 1), 1, l->type_row, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->start[0], l->start[1] - 1), 1, l->type_row, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);
	MPI_Send(l->u0 + ind(l->start[0], l->start[1]), 1, l->type_row, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->start[0], l->end[1]), 1, l->type_row, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);

    MPI_Send(l->u0 + ind(l->end[0], l->end[1] - 1), 1, MPI_INT, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->end[0], l->start[1] - 1), 1, MPI_INT, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);
	MPI_Send(l->u0 + ind(l->start[0] - 1, l->end[1] - 1), 1, MPI_INT, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->start[0] - 1, l->start[1] - 1), 1, MPI_INT, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);

	MPI_Send(l->u0 + ind(l->start[0] - 1, l->start[1]), 1, MPI_INT, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->start[0] - 1, l->end[1]), 1, MPI_INT, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);
	MPI_Send(l->u0 + ind(l->end[0], l->start[1]), 1, MPI_INT, (l->coords[1] - 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1]);
	MPI_Recv(l->u0 + ind(l->end[0], l->end[1]), 1, MPI_INT, (l->coords[1] + 1 + l->dims[1]) % l->dims[1], 0, l->comm_dims[1], MPI_STATUS_IGNORE);

	for (i = l->start[0]; i < l->end[0]; i++) {
        for (j = l->start[1]; j < l->end[1]; j++) {
			int n = 0;
			n += l->u0[ind(i + 1, j)];
			n += l->u0[ind(i + 1, j + 1)];
			n += l->u0[ind(i, j + 1)];
			n += l->u0[ind(i - 1, j)];
			n += l->u0[ind(i - 1, j - 1)];
			n += l->u0[ind(i, j - 1)];
			n += l->u0[ind(i - 1, j + 1)];
			n += l->u0[ind(i + 1, j - 1)];
			l->u1[ind(i,j)] = 0;
			if (n == 3 && l->u0[ind(i,j)] == 0) {
				l->u1[ind(i,j)] = 1;
			}
			if ((n == 3 || n == 2) && l->u0[ind(i,j)] == 1) {
				l->u1[ind(i,j)] = 1;
			}
//			l->u1[ind(i,j)] = l->rank;
		}
	}
	int *tmp;
	tmp = l->u0;
	l->u0 = l->u1;
	l->u1 = tmp;
}

void life_decomposition(int n, int size, int rank, int *start, int *end)
{
	*start = (n / size) * rank;
	*end = *start + n / size;
	if (rank == size - 1) *end = n;
}

void life_gather(life_t *l)
{
	life_gather_1d(l, l->comm_dims[0], l->block_t, ind(l->start[0], l->start[1]), ind(0, l->start[1]));
	life_gather_1d(l, l->comm_dims[1], l->rows_t, ind(0, l->start[1]), ind(0, 0));
}

void life_gather_1d(life_t *l, MPI_Comm comm, MPI_Datatype block_t, int ind_send, int ind_recv)
{
	int size;
	MPI_Comm_size(comm, &size);
	MPI_Gather(l->u0 + ind_send,
				1,
				block_t,
				l->u0 + ind_recv,
				1,
				block_t,
				size - 1,
				comm
	);
}
