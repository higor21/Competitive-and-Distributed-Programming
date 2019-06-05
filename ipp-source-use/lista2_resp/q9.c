
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void get_input_data(double* vect_1, double* vect_2, int* size, int* local_size,
	double* scalar, int my_rank, int comm_sz){

	if(!my_rank){
		printf("Informe o tamanho dos vetores: \n");
		scanf("%d", size);

		vect_1 = malloc((*size)*sizeof(double));
		vect_2 = malloc((*size)*sizeof(double));

		int i;

		printf("Digite agora os valores do vetor 1: \n");
		for (i = 0; i < (*size); ++i){
			scanf("%lf", &vect_1[i]);
		}
		printf("Digite agora os valores do vetor 2: \n");
		for (i = 0; i < (*size); ++i){
			scanf("%lf", &vect_2[i]);
		}

		printf("Informe o valor do scalar: \n");
		scanf("%lf", scalar);
	}
}

void send_data(
	double* vect_1, double* vect_2, double* local_v1, double* local_v2,
	int size, int* local_size, double* scalar, int my_rank, int comm_sz
	){

	// enviando tamanho local dos vetores
	MPI_Bcast((void*) local_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	// enviando scalar para outros processos
	MPI_Bcast((void*) scalar, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	// enviando vetor 1
	MPI_Scatter(vect_1, *local_size, MPI_DOUBLE, local_v1,
		*local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	free(vect_1);
	
	// enviando vetor 2
	MPI_Scatter(vect_2, *local_size, MPI_DOUBLE, local_v2,
		*local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	free(vect_2);
}

void process_data(double* vect_by_scl, double* dot_product, double* local_v1,
	double* local_v2, int local_size, double scalar){

	for (int i = 0; i <local_size ; ++i){
		vect_by_scl[i] = local_v1[i]*scalar;
		*dot_product += local_v1[i]*local_v2[i];
	}
}

void get_and_print_data(double* l_vect_by_scl, double* l_dot_product,
	int local_size,	double* vect_by_scl, double* dot_product, int size, int my_rank){

	// juntando os valores no vetor 'result'
	MPI_Gather(l_vect_by_scl, local_size, MPI_DOUBLE, vect_by_scl,
		local_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	MPI_Reduce((void*) l_dot_product, (void*) dot_product, 1, MPI_DOUBLE,
		MPI_SUM, 0, MPI_COMM_WORLD);

	if(!my_rank){
		printf("\nVetor x Scalar: \n");
		for (int i = 0; i < size; ++i){
			printf("%lf ", vect_by_scl[i]);
		}
		free(vect_by_scl);
		printf("\nProduto escalar: %lf", *dot_product);
	}
}

int main(void) {
	int my_rank, comm_sz;

	MPI_Init(NULL, NULL); 
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); 

	double *scalar;
	double *local_v1, *local_v2;
	int local_size;
	double *local_vect_by_scl, *local_dot_product;
	double *vect1, *vect2;
	double *vect_by_scl, *dot_product;
	int size;

	get_input_data(vect1, vect2, &size, &local_size, scalar,
	my_rank, comm_sz);
	
	local_size = size/comm_sz;

	send_data(vect1, vect2, local_v1, local_v2, size, &local_size,
		scalar, my_rank, comm_sz);

	local_vect_by_scl = malloc(local_size*sizeof(double));

	process_data(local_vect_by_scl, local_dot_product, local_v1,
		local_v2, local_size, *scalar);

	if(my_rank) vect_by_scl = malloc((local_size*comm_sz)*sizeof(double));

	get_and_print_data(local_vect_by_scl, local_dot_product,
		local_size, vect_by_scl, dot_product, size, my_rank);

	MPI_Finalize();
	return 0;
}

