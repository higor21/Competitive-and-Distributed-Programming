/* File:     mpi_output.c
 *
 * Purpose:  A program in which multiple MPI processes try to print 
 *           a message.
 *
 * Compile:  mpicc -g -Wall -o mpi_output mpi_output.c
 * Usage:    mpiexec -n<number of processes> ./mpi_output
 *
 * Input:    None
 * Output:   A message from each process
 *
 * IPP:      Section 3.3.1  (pp. 97 and ff.)
 */
#include <stdio.h>
#include <mpi.h> 

int main(void) {
	int my_rank, comm_sz;
	int id, source;

	MPI_Init(NULL, NULL); 
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz); 
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 


	if (my_rank != 0) { 
		MPI_Send(&my_rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); 
	} else {
		printf("Process %d of %d > Does anyone have a toothpick?\n", my_rank, comm_sz);
		for (source = 1; source < comm_sz; source++) {
			MPI_Recv(&id, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf("Process %d of %d > Does anyone have a toothpick?\n", id, comm_sz);
		}
	} 

	MPI_Finalize();
	return 0;
}  /* main */
