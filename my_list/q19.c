#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

void read_vector(double vect[], int *n, int my_rank);
void send_and_recv(double *array, int sz, MPI_Datatype type_to_send, MPI_Datatype type_to_recv, int my_rank, MPI_Comm comm);
void print_vector(double *vect, int n , int my_rank);
void process_vectors(int *n, int *displacements, int *blocklengths);

int main(void)
{
        int *displacements = NULL, *blocklengths = NULL, size_v;
        double *array = NULL;
        MPI_Datatype type_to_send, type_to_recv;

        int my_rank, comm_sz;
        MPI_Comm comm;

        MPI_Init(NULL, NULL);
        comm = MPI_COMM_WORLD;
        MPI_Comm_size(comm, &comm_sz);
        MPI_Comm_rank(comm, &my_rank);

        if (my_rank == 0){
                printf("Enter the order of matrix: \n");
                scanf("%d", &size_v);                
        }
        
        blocklengths = malloc(size_v * sizeof(int));
        displacements = malloc(size_v * sizeof(int));
        size_v *= size_v;
        MPI_Bcast(&size_v, 1, MPI_INT, 0, comm);
        array = malloc(size_v * sizeof(double));

        if(my_rank == 0){
                printf("\nEnter the elements of matrix:\n");

                for (int i = 0; i < size_v; i++)
                        scanf("%lf", &array[i]);
        }

        process_vectors(&size_v, displacements, blocklengths);

        // criar tipo
        MPI_Type_indexed(sqrt(size_v), blocklengths, displacements, MPI_DOUBLE, &type_to_send);
        MPI_Type_commit(&type_to_send);


        // calculo do tamanho da saída
        int sz_out = 0;
        for(int i = 0; i < sqrt(size_v) ; i++)
                sz_out += blocklengths[i];

        MPI_Type_contiguous(sz_out, MPI_DOUBLE, &type_to_recv);
        MPI_Type_commit(&type_to_recv);

        // send and recv
        send_and_recv(array, size_v, type_to_send, type_to_recv, my_rank, comm);

        // print
        print_vector(array, sz_out, my_rank);

	/* Shut down MPI */
	MPI_Finalize();

}

void process_vectors(int *n, int *displacements, int *blocklengths){

        int curr_length = sqrt(*n);
        int block = sqrt(*n);
        int iter = 0, k = 0, i;
        int vb = 1;
        
        for (i = 0; i < *n; i++)
        {
                if (i < block)
                {
                        if (vb)
                        {
                                displacements[k] = i;
                                blocklengths[k++] = curr_length--;
                                vb = 0;
                        }
                }
                else
                {
                        i += iter++;
                        block += sqrt(*n);
                        vb = 1;
                }
        }
}

void send_and_recv(double *array, int sz, MPI_Datatype type_to_send, MPI_Datatype type_to_recv, int my_rank, MPI_Comm comm){
        if (my_rank == 0) { 
		MPI_Send(array, 1, type_to_send, 1, 0, comm);
	}
        
        if(my_rank == 1){
	        MPI_Recv(array, 1, type_to_recv, 0, 0, comm, MPI_STATUS_IGNORE);
	}
}

void print_vector(double *vect, int n , int my_rank){
        if (my_rank == 1)
        {
                printf("result: ");
                for (int i = 0; i < n; i++)
                        printf("%lf ", vect[i]);
                printf("\n");
        }
}
