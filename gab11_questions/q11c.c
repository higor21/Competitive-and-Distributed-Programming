// Compile:  mpicc -g -Wall -o q11c q11c.c -lm
// Run:      mpiexec -n <number of processes> ./q11c

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

void receive_element(int *my_xi, int my_rank, int comm_sz);
double log2(double n);  

int main(void) {
   int my_rank, comm_sz, my_xi;
   int local_sum, auxiliar, iterations;
   int rcv_partner, snd_partner;

   /* Inicialização do MPI */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

   /* Cálculo do número de iterações */
   iterations = (int) ceil(log2(comm_sz));

   /* Inicializa a soma prefixada de cada processo com o próprio elemento que cada processo armazena */
   receive_element(&my_xi, my_rank, comm_sz);
   local_sum = my_xi;

   for(int i=0; i<iterations; i++){
      /* O processo deve mandar sua soma prefixada parcial para snd_partner */
      if(my_rank < (comm_sz - pow(2,i))){ 
         snd_partner = my_rank + pow(2,i);
         MPI_Send(&local_sum, 1, MPI_INT, snd_partner, 0, MPI_COMM_WORLD); 
      }
      /* O processo deve receber a soma prefixada parcial de rcv_partner e adicioná-la à sua própria soma */ 
      if(my_rank >= pow(2,i)){
         rcv_partner = my_rank - pow(2,i);
         MPI_Recv(&auxiliar, 1, MPI_INT, rcv_partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         local_sum += auxiliar;
      }
   }

   /* Resultados */
   printf("Sou o processo %d e minha soma prefixada é %d!\n", my_rank, local_sum);
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

double log2(double n)  
{  
    return log(n) / log(2);  
} 