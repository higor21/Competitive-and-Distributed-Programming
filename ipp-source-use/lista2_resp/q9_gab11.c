#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

const int MAX_LENGTH = 1024;

void receive_length(int *n, int *local_n, int my_rank, int comm_sz);
void receive_input(double local_v1[], double local_v2[], int local_n, int n, double *scalar, int my_rank, int comm_sz, MPI_Comm comm);
void scalar_dot_product(double local_v1[], double local_v2[], int local_n, double scalar, double result[]);
void print_result(double local_result[], int local_n, int n, int my_rank, MPI_Comm comm);

int main(void) {
   int my_rank, comm_sz, n, local_n;  
   double scalar, local_v1[MAX_LENGTH], local_v2[MAX_LENGTH], local_result[MAX_LENGTH];

   /* Inicialização do MPI */
   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

   /* Processo 0 recebe o tamanho do problema e informa a todos o tamanho do bloco na distribuição*/
   receive_length(&n, &local_n, my_rank, comm_sz);

   /* Processo 0 recebe os inputs e distribui uma parte para os outros, usando Scatter */
   receive_input(local_v1, local_v2, local_n, n, &scalar, my_rank, comm_sz, MPI_COMM_WORLD);

   /* Cada processo realiza a sua computação parcial do produto */
   scalar_dot_product(local_v1, local_v2, local_n, scalar, local_result);

   /* Todos os processos distribuem seus resultados parciais para o processo 0, usando Gather */
   print_result(local_result, local_n, n, my_rank, MPI_COMM_WORLD);

   /* Finalização do MPI */
   MPI_Finalize();
   return 0;
} 

void receive_length(int *n, int *local_n, int my_rank, int comm_sz){
   if(my_rank == 0){
      printf("Entre com o tamanhos dos vetores (n):\n");
      scanf("%d", n);
      *local_n = *n/comm_sz; 
   }
   MPI_Bcast(local_n, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

void receive_input(double local_v1[], double local_v2[], int local_n, int n, double *scalar, int my_rank, int comm_sz, MPI_Comm comm){
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
      printf("Entre com o escalar (scalar):\n");
      scanf("%lf", scalar);
      MPI_Scatter(v1, local_n, MPI_DOUBLE, local_v1, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      free(v1);
      MPI_Scatter(v2, local_n, MPI_DOUBLE, local_v2, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      free(v2);
   }else{
      MPI_Scatter(v1, local_n, MPI_DOUBLE, local_v1, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      MPI_Scatter(v2, local_n, MPI_DOUBLE, local_v2, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   }
   MPI_Bcast(scalar, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

void scalar_dot_product(double local_v1[], double local_v2[], int local_n, double scalar, double local_result[]){
   for(int i=0; i<local_n; i++){
      local_result[i] = local_v1[i]*scalar*local_v2[i];
   }
}

void print_result(double local_result[], int local_n, int n, int my_rank, MPI_Comm comm){
   double* result = NULL;
   if(my_rank == 0){
      result = malloc(n*sizeof(double));
      MPI_Gather(local_result, local_n, MPI_DOUBLE, result, local_n, MPI_DOUBLE, 0, comm);
      printf("O vetor resultante foi: \n");
      for(int i=0; i<n; i++){
         printf("%lf ", result[i]);
      }
      free(result);
   }else{
      MPI_Gather(local_result, local_n, MPI_DOUBLE, result, local_n, MPI_DOUBLE, 0, comm);
   }
}