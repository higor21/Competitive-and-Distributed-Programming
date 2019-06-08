#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

void read_vector(double vect[], int *n, int my_rank);
void send_and_recv(double *array, int size_v, MPI_Datatype *new_type, int my_rank, MPI_Comm comm);
void print_vector(double *vect, int n , int my_rank);
void iprint_vector(int *vect, int n , int my_rank);
void read_vector(double *vect, int *n, int my_rank);
void process_vectors(double *vect, int *n, int *displacements, int *blocklengths, int my_rank);

int main(void)
{
        int *displacements = NULL, *blocklengths = NULL, size_v;
        double *array = NULL;
        MPI_Datatype *new_type = NULL;

        int my_rank, comm_sz;
        MPI_Comm comm;

        MPI_Init(NULL, NULL);
        comm = MPI_COMM_WORLD;
        MPI_Comm_size(comm, &comm_sz);
        MPI_Comm_rank(comm, &my_rank);

        // ler vetor
        //read_vector(array, &size_v, my_rank);
        
        if (my_rank == 0){
                printf("Enter the order of matrix: \n");
                scanf("%d", &size_v);

                blocklengths = malloc(size_v * sizeof(int));
                displacements = malloc(size_v * sizeof(int));
                
                size_v *= size_v;

                array = malloc(size_v * sizeof(double));

                printf("\nEnter the elements of matrix:\n");

                for (int i = 0; i < size_v; i++)
                        scanf("%lf", &array[i]);

                // calcular displac.. e block...
                process_vectors(array, &size_v, displacements, blocklengths, my_rank);

                //print_vector(array, size_v, 1);
                //iprint_vector(displacements, sqrt(size_v), 1);
                //iprint_vector(blocklengths, sqrt(size_v), 1);
                
                // criar tipo
                MPI_Type_indexed(sqrt(size_v), blocklengths, displacements, MPI_DOUBLE, new_type);
                MPI_Type_commit(new_type);
        }

        // send and recv
        //send_and_recv(array, size_v, new_type, my_rank, comm);

        // print
        //print_vector(array, size_v, my_rank);

	/* Shut down MPI */
	MPI_Finalize();

}

void read_vector(double *vect, int *n, int my_rank)
{
        if (my_rank == 0)
        {
                printf("Enter the order of matrix: ");
                scanf("%d", n);

                *n *= *n;

                vect = malloc((*n) * sizeof(double));

                printf("\nEnter the elements of vector:\n");

                for (int i = 0; i < *n; i++)
                        scanf("%lf", &vect[i]);
        }
}

void process_vectors(double *vect, int *n, int *displacements, int *blocklengths, int my_rank){

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

void send_and_recv(double *array, int size_v, MPI_Datatype *new_type, int my_rank, MPI_Comm comm){
        if (my_rank == 0) { 
		MPI_Send(array, 1, *new_type, 1, 0, comm);
	}
        
        if(my_rank == 1){
	     MPI_Recv(array, 1, *new_type, 0, 0, comm, MPI_STATUS_IGNORE);
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

void iprint_vector(int *vect, int n , int my_rank){
        if (my_rank == 1)
        {
                printf("result: ");
                for (int i = 0; i < n; i++)
                        printf("%d ", vect[i]);
                printf("\n");
        }
}