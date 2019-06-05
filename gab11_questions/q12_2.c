// Compile:  mpicc -g -Wall -o q12_2 q12_2.c -lm
// Run:      mpiexec -n <number of processes> ./q12_2

#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>

const int MAX = 10;
const int MAX_LENGTH = 1024;

void generate_random_array(int array[], int comm_sz);  
double log2(double n);

int main(void) {
   double local_start, local_finish, local_elapsed, elapsed;
   int my_rank, comm_sz, array[MAX_LENGTH];
   int temp_value, sum; 
   int source, dest, iterations;

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

   temp_value = sum = array[my_rank];
   iterations = (int) ceil(log2(comm_sz));

   for(int i=0; i<iterations; i++){
      if(my_rank % (int) pow(2, i+1) < (int) pow(2, i+1)/2)
         source = dest = (my_rank + (int) pow(2,i));
      else
         source = dest = (my_rank - (int) pow(2,i));
      MPI_Sendrecv_replace(&temp_value, 1, MPI_INT, dest, 0, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      sum += temp_value;
      temp_value = sum;
   }
   /* Fim da computação */

   local_finish = MPI_Wtime();
   local_elapsed = local_finish - local_start;
   MPI_Reduce(&local_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

   /* Resultados */
   if(my_rank == 0){
      printf("Elapsed time = %e seconds\n", elapsed);
      /*
      printf("Array original: ");
      for(int i=0; i<comm_sz; i++){
         printf("%d ", array[i]);
      }
      printf("\n");
      */
   }
   /*
   MPI_Barrier(MPI_COMM_WORLD);
   printf("I am process %d and my global sum is %d!\n", my_rank, sum);
   */

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

double log2( double n )  
{  
    return log( n ) / log( 2 );  
}  