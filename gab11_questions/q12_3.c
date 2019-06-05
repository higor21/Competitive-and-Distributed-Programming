// Compile:  mpicc -g -Wall -o q12_3 q12_3.c 
// Run:      mpiexec -n <number of processes> ./q12_3

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int MAX = 10;
const int MAX_LENGTH = 1024;

void generate_random_array(int array[], int comm_sz);  

int main(void) {
   double local_start, local_finish, local_elapsed, elapsed;
   int my_rank, comm_sz, array[MAX_LENGTH];
   int temp_value, sum; 
   int source, dest;

   /* Inicialização do MPI */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

   /* Geração do vetor com comm_sz elementos (1 para cada processo) */
   generate_random_array(array, comm_sz);

   /* Contagem do tempo deve começar aproximadamente no mesmo tempo para os processos */
   MPI_Barrier(MPI_COMM_WORLD);

   /* Início da computação */
   local_start = MPI_Wtime();

   sum = temp_value = array[my_rank];
   source = (my_rank + comm_sz - 1) % comm_sz;
   dest = (my_rank + 1) % comm_sz;

   for(int i=1; i<comm_sz; i++){
      if(i <= my_rank){
         MPI_Sendrecv_replace(&temp_value, 1, MPI_INT, dest, 0, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         sum += temp_value;
      }
      else
         MPI_Send(&temp_value, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
   }
   /* Fim da computação */

   local_finish = MPI_Wtime();
   local_elapsed = local_finish - local_start;
   MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

   /* Resultados */
   if(my_rank == 0){
      printf("Elapsed time = %e seconds\n", elapsed);
      
      printf("Array original: ");
      for(int i=0; i<comm_sz; i++){
         printf("%d ", array[i]);
      }
      printf("\n");
      
   }
   
   MPI_Barrier(MPI_COMM_WORLD);
   printf("I am process %d and my prefix sum is %d!\n", my_rank, sum);
   

   /* Finalização do MPI */
   MPI_Finalize();

   return 0;
}

void generate_random_array(int array[], int comm_sz){
   for(int i=0; i<comm_sz; i++){
      /* Número aleatório entre 1 e MAX+1 */
      array[i] = (rand() % MAX + 1);
   }
}