// Compile:  mpicc -g -Wall -o q11b q11b.c
// Run:      mpiexec -n <number of processes> ./q11b

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void receive_element(int *my_xi, int my_rank, int comm_sz);

int main(void) {
   int my_rank, comm_sz;
   int my_xi, temp, sum;

   /* Inicialização do MPÍ */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);


   receive_element(&my_xi, my_rank, comm_sz);
   sum = my_xi;


   for(int i=my_rank+1; i<comm_sz; i++){
      MPI_Send(&my_xi, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
   }
   for(int i=0; i<my_rank; i++){
      MPI_Recv(&temp, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      sum += temp;
   }

   /* Resultados */
   printf("Sou o processo %d e minha soma eh %d!\n", my_rank, sum);
   MPI_Finalize();
   return 0;
}

void receive_element(int *my_xi, int my_rank, int comm_sz){
   int *array = NULL;

   if(my_rank == 0){
      array = malloc(comm_sz*sizeof(int));
      printf("\nVetor gerado: ");
      for(int i=0; i<comm_sz; i++){
         array[i] = (rand() % 100) + 1;
         printf("%d ", array[i]);
      }
      printf("\n");
      MPI_Scatter(array, 1, MPI_INT, my_xi, 1, MPI_INT, 0, MPI_COMM_WORLD);
      free(array);
   }
   else
      MPI_Scatter(array, 1, MPI_INT, my_xi, 1, MPI_INT, 0, MPI_COMM_WORLD);
}
