// Compile:  mpicc -g -Wall -o q17 q17.c
// Run:      mpiexec -n <comm_sz> ./q17

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void Build_mpi_type(MPI_Datatype *old_type);
void Read_n(int* n_p, int* local_n_p, int my_rank, int comm_sz);
void Allocate_vectors(double** local_x_pp, int local_n);
void Read_vector(double local_x[], int local_n, int n, int my_rank, MPI_Datatype *new_type);
void Print_vector(double local_x[], int local_n, int n, int my_rank, MPI_Datatype *new_type);

int main(void) {
   MPI_Datatype *new_type;
   int n, local_n;
   int comm_sz, my_rank;
   double *local_x;

   MPI_Init(NULL, NULL);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

   Read_n(&n, &local_n, my_rank, comm_sz);

   Allocate_vectors(&local_x, local_n);
   
   Read_vector(local_x, local_n, n, my_rank, new_type);
   Print_vector(local_x, local_n, n, my_rank, new_type);

   MPI_Finalize();

   return 0;
}  /* main */

void Read_n(int* n_p, int* local_n_p, int my_rank, int comm_sz) {

   if (my_rank == 0) {
      printf("What's the order of the vectors?\n");
      scanf("%d", n_p);
   }
   MPI_Bcast(n_p, 1, MPI_INT, 0, MPI_COMM_WORLD);
   *local_n_p = *n_p/comm_sz;
}  /* Read_n */

void Allocate_vectors(double** local_x_pp, int local_n){

   *local_x_pp = malloc(local_n*sizeof(double));

} 

void Build_mpi_type(MPI_Datatype *single_input_type){
   int array_of_blocklengths[1] = {1};
   MPI_Datatype array_of_types[1] = {MPI_DOUBLE};
   MPI_Aint array_of_displacements[1] = {0};

   MPI_Type_create_struct(1, array_of_blocklengths, array_of_displacements, array_of_types, single_input_type);

}

void Read_vector(double local_x[], int local_n, int n, int my_rank, MPI_Datatype *new_type) {

   double* a = NULL;
   int i;

   MPI_Type_contiguous(local_n, MPI_DOUBLE, new_type);
   MPI_Type_commit(new_type);

   if (my_rank == 0) {
      a = malloc(n*sizeof(double));
      printf("Enter the array: ");
      for (i = 0; i < n; i++)
         scanf("%lf", &a[i]);
      MPI_Scatter(a, 1, *new_type, local_x, 1, *new_type, 0, MPI_COMM_WORLD);
      free(a);
   } 
   else {
      MPI_Scatter(a, 1, *new_type, local_x, 1, *new_type, 0, MPI_COMM_WORLD);
   }
}  /* Read_vector */  

void Print_vector(double local_x[], int local_n, int n, int my_rank, MPI_Datatype *new_type) {

   double* b = NULL;
   int i;

   if (my_rank == 0) {
      b = malloc(n*sizeof(double));

      MPI_Gather(local_x, 1, *new_type, b, 1, *new_type, 0, MPI_COMM_WORLD);
      printf("\nFinal vector is: ");
      for (i = 0; i < n; i++)
         printf("%lf ", b[i]);
      printf("\n");
      free(b);
   } else {
      MPI_Gather(local_x, 1, *new_type, b, 1, *new_type, 0, MPI_COMM_WORLD);
   }
   MPI_Type_free(new_type);
}  /* Print_vector */

