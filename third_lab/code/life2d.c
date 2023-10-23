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

	int rank;
	int size;
	int start, end;
	MPI_Datatype type_col, type_block;
} life_t;

void life_init(const char *path, life_t *l);
void life_free(life_t *l);
void life_step(life_t *l);
void life_save_vtk(const char *path, life_t *l);
void life_gather(life_t *l);
void life_gather2(life_t *l);

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s <input file> <output file>.\n", argv[0]);
		return 0;
	}
	MPI_Init(&argc, &argv);
	life_t l;
	double start_time;
    double end_time;

	life_init(argv[1], &l);
	if (l.rank == 0) {
		start_time = MPI_Wtime();
	}
	
	int i;
	char buf[100];
	for (i = 0; i < l.steps; i++) {
		// if (i % l.save_steps == 0) {
		// 	sprintf(buf, "result/life_%06d.vtk", i);
		// 	// printf("Saving step %d to '%s'.\n", i, buf);
		// 	life_gather2(&l);
		// 	if (l.rank == l.size - 1)
		// 		life_save_vtk(buf, &l);
		// }
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

	/* Decompozition. */
	MPI_Comm_size(MPI_COMM_WORLD, &(l->size));
	MPI_Comm_rank(MPI_COMM_WORLD, &(l->rank));

	l->start = l->rank * (l->nx / l->size);
	l->end = (l->rank + 1) * (l->nx / l->size);
	if (l->rank == l->size - 1) l->end = l->nx;

	printf("#%d: start = %d, end = %d\n", l->rank, l->start, l->end);

	MPI_Type_vector(l->ny, 1, l->nx, MPI_INT, &(l->type_col));
	MPI_Type_commit(&(l->type_col));

	if (l->rank != l->size - 1) {
		MPI_Type_vector(l->ny, l->end - l->start, l->nx, MPI_INT, &(l->type_block));
	} else {
		int start = i * (l->nx / l->size);
		int end = (i + 1) * (l->nx / l->size);
		MPI_Type_vector(l->ny, end - start, l->nx, MPI_INT, &(l->type_block));
	}
	
	MPI_Type_commit(&(l->type_block));
}

void life_free(life_t *l)
{
	free(l->u0);
	free(l->u1);
	l->nx = l->ny = 0;
	MPI_Type_free(&(l->type_col));
	MPI_Type_free(&(l->type_block));
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

	MPI_Send(l->u0 + ind(l->end - 1, 0), 1, l->type_col, (l->rank + 1 + l->size) % l->size, 0, MPI_COMM_WORLD);
	MPI_Recv(l->u0 + ind(l->start - 1, 0), 1, l->type_col, (l->rank - 1 + l->size) % l->size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Send(l->u0 + ind(l->start, 0), 1, l->type_col, (l->rank - 1 + l->size) % l->size, 0, MPI_COMM_WORLD);
	MPI_Recv(l->u0 + ind(l->end, 0), 1, l->type_col, (l->rank + 1 + l->size) % l->size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	for (j = 0; j < l->ny; j++) {
		for (i = l->start; i < l->end; i++) {
			int n = 0;
			n += l->u0[ind(i+1, j)];
			n += l->u0[ind(i+1, j+1)];
			n += l->u0[ind(i,   j+1)];
			n += l->u0[ind(i-1, j)];
			n += l->u0[ind(i-1, j-1)];
			n += l->u0[ind(i,   j-1)];
			n += l->u0[ind(i-1, j+1)];
			n += l->u0[ind(i+1, j-1)];
			l->u1[ind(i,j)] = 0;
			if (n == 3 && l->u0[ind(i,j)] == 0) {
				l->u1[ind(i,j)] = 1;
			}
			if ((n == 3 || n == 2) && l->u0[ind(i,j)] == 1) {
				l->u1[ind(i,j)] = 1;
			}
			// l->u1[ind(i,j)] = l->rank;
		}
	}
	int *tmp;
	tmp = l->u0;
	l->u0 = l->u1;
	l->u1 = tmp;
}

void life_gather(life_t *l)
{
	if (l->rank == 0) {
		int i;
		for (i = 1; i < l->size; i++) {
			int start = (i * l->ny) / l->size;
			int end = ((i + 1) * l->ny) / l->size;
			MPI_Recv(l->u0 + ind(0, start), (end-start) * l->nx,
			MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	} else {
		MPI_Send(l->u0 + ind(0, l->start),
		 (l->end - l->start) * l->nx, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}
}

void life_gather2(life_t *l)
{
	if (l->rank == l->size - 1) {
		int i;
		for (i = 0; i < l->size-1; i++) {
			int start = i * (l->nx / l->size);
			int end = (i + 1) * (l->nx / l->size);
			MPI_Recv(l->u0 + ind(start, 0), 1,
			l->type_block, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
	} else {
		MPI_Send(l->u0 + ind(l->start, 0),
		 1, l->type_block, l->size - 1, 0, MPI_COMM_WORLD);
	}
}

