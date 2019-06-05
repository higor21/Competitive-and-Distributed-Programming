// Compile:  mpicc -g -Wall -o q9 q9.c
// Run:      mpiexec -n <number of processes> ./q9

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void recv_sz_and_dist(int *n, int *local_n, int my_rank, int comm_sz);
void recv_ipts_and_dist(double local_v1[], double local_v2[], int local_n, int n, double *scalar, int my_rank);
double vect_dot_product(double local_v1[], double local_v2[], int local_n);
void scalar_prod(double local_v1[], double local_v3[], int local_n, double scalar);
void send_results_and_print(double local_v3[], int local_n, int n, double local_result, int my_rank);

int main(void) {
   int my_rank, comm_sz, n, local_n;  
   double scalar, local_result;
   double *local_v1 = NULL, *local_v2 = NULL, *local_v3 = NULL;


   MPI_Init(NULL, NULL);
   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    // cada valor indica a posição inicial dos vetores locais
   int *displs = malloc(sizeof(int)*comm_sz);
   // cada valor indica a quantidade de elementos do vetor local do processo
   int *sendcounts = malloc(sizeof(int)*comm_sz);

   recv_sz_and_dist(&n, &local_n, my_rank, comm_sz);

   local_v1 = malloc(local_n*sizeof(double));
   local_v2 = malloc(local_n*sizeof(double));
   local_v3 = malloc(local_n*sizeof(double));

   recv_ipts_and_dist(local_v1, local_v2, local_n, n, &scalar, my_rank);

   local_result = vect_dot_product(local_v1, local_v2, local_n);

   scalar_prod(local_v1, local_v3, local_n, scalar);

   send_results_and_print(local_v3, local_n, n, local_result, my_rank);

   free(local_v1);
   free(local_v2);
   free(local_v3);
   
   MPI_Finalize();
   return 0;
} 

void recv_sz_and_dist(int *n, int *local_n, int my_rank, int comm_sz){
   if(my_rank == 0){
      printf("Entre com o tamanhos dos vetores (n):\n");
      scanf("%d", n);
      *local_n = *n/comm_sz; 
   }
   MPI_Bcast(local_n, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

void recv_ipts_and_dist(double local_v1[], double local_v2[], int local_n,
                                    int n, double *scalar, int my_rank){
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
   }
   
   MPI_Scatter(v1, local_n, MPI_DOUBLE, local_v1, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   free(v1);
   MPI_Scatter(v2, local_n, MPI_DOUBLE, local_v2, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   free(v2);
   MPI_Bcast(scalar, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

void scalar_prod(double local_v1[], double local_v3[], int local_n,
                           double scalar){
   for(int i=0; i<local_n; i++){
      local_v3[i] = local_v1[i]*scalar;
   }
}

double vect_dot_product(double local_v1[], double local_v2[], int local_n){
   double sum = 0;
   for(int i=0; i<local_n; i++){
      sum += local_v1[i]*local_v2[i];
   }
   return sum;
}

void send_results_and_print(double local_v3[], int local_n, int n, double local_result,
                              int my_rank){
   double result;
   double* v3 = NULL;
   MPI_Reduce(&local_result, &result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
   if(my_rank == 0){
      v3 = malloc(n*sizeof(double));
      MPI_Gather(local_v3, local_n, MPI_DOUBLE, v3, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      printf("A multiplicação de v1 pelo escalar gerou: \n");
      for(int i=0; i<n; i++){
         printf("%lf ", v3[i]);
      }
      free(v3);
      printf("\nO produto escalar de v1 com v2 foi: %lf\n", result);
   }
   else{
      MPI_Gather(local_v3, local_n, MPI_DOUBLE, v3, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
   }
}

