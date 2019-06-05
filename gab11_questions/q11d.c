// Compile:  mpicc -g -Wall -o q11d q11d.c 
// Run:      mpiexec -n <number of processes> ./q11c

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>

const int MAX = 100;
const int count = 5;

void generate_random_array(int array[], int count, int my_rank);  

int main(void) {
   int my_rank, comm_sz;
   int array[count], prefix_sum[count];

   /* Inicialização do MPÍ */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);


   /* Geração de vetor aleatório */
   generate_random_array(array, count, my_rank);

   /* Redução aplicada nos elementos com índice menor que my_rank */
   MPI_Scan(array, prefix_sum, count, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

   printf("Prefix sum do processo %d: ", my_rank);
   for(int i=0; i<count; i++){
      printf("%d ", prefix_sum[i]);
   }
   printf("\n");

   MPI_Finalize();
   return 0;
}


void generate_random_array(int array[], int count, int my_rank){
   srand(my_rank*time(NULL));
   printf("Array do processo %d: ", my_rank);
   for(int i=0; i<count; i++){
      /* Número aleatório entre 1 e MAX+1 */
      array[i] = (rand() % MAX + 1);
      printf("%d ", array[i]);
   }
   printf("\n");
}