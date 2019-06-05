// Compile:  mpicc -g -Wall -o q13 q13.c
// Run:      mpiexec -n <number of processes> ./q13

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void configure_distribution(int *n, int *sendcnts, int *displs, int my_rank, int comm_sz);
void distribute_inputs(double *local_v1, double *local_v2, int local_n, int n, int *sendcnts, int *displs, int my_rank);
void vect_sum(double *local_v1, double *local_v2, double *local_v3, int local_n);
double vect_dot_product(double *local_v1, double *local_v2, int local_n);
void send_results_and_print(double *local_v3, int local_n, int n, int *sendcnts, int *displs, double local_result, int my_rank);

int main(void) {
   int my_rank, comm_sz, n=0, local_n; 
   int *sendcnts = NULL, *displs = NULL;
   double local_result;
   double *local_v1 = NULL, *local_v2 = NULL, *local_v3 = NULL;

   /* Inicialização do MPI */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

   /* Sendcnts e displs são arrays que especificam quantos e quais elementos cada processo pegará */
   sendcnts = malloc(comm_sz*sizeof(int));
   displs = malloc(comm_sz*sizeof(int));

   /* Processo 0 decide como será feita a distribuição, preenche sendcnts e displs e os manda para os outros processos*/
   configure_distribution(&n, sendcnts, displs, my_rank, comm_sz);

   /* Após saber quantos elementos cada processo lidará, os mesmos alocam os espaços necessários */
   local_n = sendcnts[my_rank];
   local_v1 = malloc(local_n*sizeof(double));
   local_v2 = malloc(local_n*sizeof(double));
   local_v3 = malloc(local_n*sizeof(double));

   /* Processo 0 recebe os vetores de entrada e distribui os elementos para os outros processos */
   distribute_inputs(local_v1, local_v2, local_n, n, sendcnts, displs, my_rank);

   /* Cada processo realiza a soma de v1 com v2 e armazena em v3 */
   vect_sum(local_v1, local_v2, local_v3, local_n);

   /* Cada processo realiza o produto escalar de v1 com v2 e armazena em result */
   local_result = vect_dot_product(local_v1, local_v2, local_n);


   /* Todos os processos distribuem seus resultados parciais para o processo 0, usando Gatherv e Reduce */
   send_results_and_print(local_v3, local_n, n, sendcnts, displs, local_result, my_rank);

   free(local_v1);
   free(local_v2);
   free(local_v3);
   /* Finalização do MPI */
   MPI_Finalize();
   return 0;
} 


void configure_distribution(int *n, int *sendcnts, int *displs, int my_rank, int comm_sz){
   int aux;
   if(my_rank == 0){

      printf("Entre com o tamanhos dos vetores (n):\n");
      scanf("%d", n);

      aux = *n/comm_sz + 1;
      
      for(int i=0; i<comm_sz; i++){
         if(i==0)
            displs[i] = 0;
         else
            displs[i] = displs[i-1] + sendcnts[i-1];
         if(i < *n%comm_sz)
            sendcnts[i] = aux;
         else
            sendcnts[i] = aux - 1;
      }

   }

   MPI_Bcast(sendcnts, comm_sz, MPI_INT, 0, MPI_COMM_WORLD);
   MPI_Bcast(displs, comm_sz, MPI_INT, 0, MPI_COMM_WORLD);
}

void distribute_inputs(double *local_v1, double *local_v2, int local_n, int n, int *sendcnts, int *displs, int my_rank){
   double* v1 = NULL;
   double* v2 = NULL;
   if(my_rank == 0){
      v1 = malloc(n*sizeof(double));
      printf("Entre com os elementos do primeiro vetor (v1):\n");
      for(int i=0; i<n; i++){
         scanf("%lf", &v1[i]);
      }
      v2 = malloc(n*sizeof(double));
      printf("Entre com os elementos do segundo vetor (v2):\n");
      for(int i=0; i<n; i++){
         scanf("%lf", &v2[i]);
      }
      MPI_Scatterv(v1, sendcnts, displs, MPI_DOUBLE, local_v1, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      free(v1);
      MPI_Scatterv(v2, sendcnts, displs, MPI_DOUBLE, local_v2, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      free(v2);
   }
   else{
      MPI_Scatterv(v1, sendcnts, displs, MPI_DOUBLE, local_v1, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      MPI_Scatterv(v2, sendcnts, displs, MPI_DOUBLE, local_v2, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   }
}

void vect_sum(double *local_v1, double *local_v2, double *local_v3, int local_n){
   for(int i=0; i<local_n; i++){
      local_v3[i] = local_v1[i] + local_v2[i];
   }
}

double vect_dot_product(double *local_v1, double *local_v2, int local_n){
   double sum = 0;
   for(int i=0; i<local_n; i++){
      sum += local_v1[i]*local_v2[i];
   }
   return sum;
}



void send_results_and_print(double *local_v3, int local_n, int n, int *sendcnts, int *displs, double local_result, int my_rank){
   double result;
   double* v3 = NULL;
   MPI_Reduce(&local_result, &result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   if(my_rank == 0){
      v3 = malloc(n*sizeof(double));
      MPI_Gatherv(local_v3, local_n, MPI_DOUBLE, v3, sendcnts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      printf("A soma de v1 com v2 gerou: \n");
      for(int i=0; i<n; i++){
         printf("%lf ", v3[i]);
      }
      free(v3);
      printf("\nO produto escalar de v1 com v2 foi: %lf\n", result);
   }
   else{
      MPI_Gatherv(local_v3, local_n, MPI_DOUBLE, v3, sendcnts, displs, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   }
}

