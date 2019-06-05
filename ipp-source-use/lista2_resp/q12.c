#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

void ring_pass_function(int my_val, int my_rank, int comm_sz, char* my_msg){
    int sum, temp_val, source, dest; 
    int p = comm_sz;

    dest = (my_rank == p - 1) ? 0 : my_rank + 1;
    source = (my_rank == 0) ? p - 1 : my_rank - 1;


    // MUDANÇAS

    // cada processo inicia com seu valor SUM incluso
    sum = temp_val = my_val;
    
    for (int i = 1; i < p; i++){

        // não muda nada, visto que a lógica é a mesma
        // para essa função
        MPI_Sendrecv_replace(
            &temp_val       ,// buf_p 
            1               ,// buf_size
            MPI_INT         ,// buf_type
            dest,
            0               ,// send_tag,
            source,
            0               ,// recvtag
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE
        );
    
        // cada processo, considera apenas o valor 
        // do processo cujo o rank é menor que o seu
        if(my_rank >= i)
            sum += temp_val;
    }
    sprintf(my_msg, "my_rank: %d  >>\t\tmy_val = %d \t,   my_sum = %d\n", my_rank, my_val, sum);
}
/*
void ring_pass_function(int my_val, int my_rank, int comm_sz, char* my_msg){
    int sum, temp_val, source, dest; 
    int p = comm_sz;
    printf("sdfasdf");
    sum = 0;
    temp_val = my_val;
    
    for (int i = 0; i < p; i++){
        printf("heloo");
        dest = (my_rank + i + 1)%p;
        source = i;

        //sprintf(my_msg, "my_rank: %d  >>\t src: %d\t,  dest: %d", my_rank, source, dest);
        //printf(my_msg);

        MPI_Sendrecv_replace(
            &temp_val      ,
            1              ,
            MPI_INT        ,
            dest,
            0              ,
            source,
            0              ,
            MPI_COMM_WORLD,
            MPI_STATUS_IGNORE
        );

        if(my_rank >= i)
            sum += temp_val;
    }
    sprintf(my_msg, "my_rank: %d  >>\t\tmy_val = %d \t,   my_sum = %d\n", my_rank, my_val, sum);
}*/


int generate_value(){
    return rand()%300;
}

int main(void) {
	int my_rank, comm_sz;
    srand(my_rank*time(NULL));

	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    int length = 100;
    char *msg = (char *) malloc(length);
    ring_pass_function(generate_value(), my_rank, comm_sz, msg);

    char *msg_recv = (char *)malloc(length+1);
    
    if (my_rank) { 
		MPI_Send(msg, length , MPI_CHAR, 0, 0, MPI_COMM_WORLD);
	} else {
		printf(msg);
		for (int src = 1; src < comm_sz; src++) {
			MPI_Recv(msg_recv, length+1, MPI_CHAR, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printf(msg_recv);
		}
	}

	MPI_Finalize();
	return 0;
}