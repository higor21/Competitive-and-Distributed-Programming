// Compile:  mpicc -g -Wall -o teste teste.c
// Run:      mpiexec -n <comm_sz> ./teste

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void Build_mpi_type(double *a, double *b, int *n, MPI_Datatype *input_mpi_t_p);
void Get_input(int my_rank, int comm_sz, double *a, double *b, int *n);

int main(void) {
   int my_rank, comm_sz, n;
   double a, b;

   MPI_Init(NULL, NULL);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

   Get_input(my_rank, comm_sz, &a, &b, &n);

   printf("\nI am %d and my %d is: ", my_rank, n);

   MPI_Finalize();

   return 0;
}  /* main */

void Build_mpi_type(double *a, double *b, int *n, MPI_Datatype *input_mpi_t_p){
	int array_of_blocklengths[3] = {1, 1, 1};
	MPI_Datatype array_of_types[3] = {MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
	MPI_Aint a_addr, b_addr, n_addr;
	MPI_Aint array_of_displacements[3] = {0};

	MPI_Get_address(a, &a_addr);
	MPI_Get_address(b, &b_addr);
	MPI_Get_address(n, &n_addr);

	array_of_displacements[1] = b_addr - a_addr;
	array_of_displacements[2] = n_addr - a_addr;

	MPI_Type_create_struct(3, array_of_blocklengths, array_of_displacements, array_of_types, input_mpi_t_p);

	MPI_Type_commit(input_mpi_t_p);
}

void Get_input(int my_rank, int comm_sz, double *a, double *b, int *n){
	MPI_Datatype input_mpi_t;

	Build_mpi_type(a, b, n, &input_mpi_t);

	if(my_rank == 0){
		printf("Enter a, b and n\n");
		scanf("%lf %lf %d", a, b, n);
	}
	MPI_Bcast(a, 1, input_mpi_t, 0, MPI_COMM_WORLD);
	MPI_Type_free(&input_mpi_t);
}