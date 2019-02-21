#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <math.h>
#include "mpi.h"

#define MASTER 0
#define TAG_HELLO 4
#define TAG_TEST 5
#define TAG_TIME 6

int main(int argc, char *argv[])
{
 int i, id, remote_id, num_procs;

 MPI_Status stat;
 int namelen;
 char name[MPI_MAX_PROCESSOR_NAME];
 // Start MPI.
 if (MPI_Init (&argc, &argv) != MPI_SUCCESS)
 {
 printf ("Failed to initialize MPI\n");
 return (-1);
 }

 // Create the communicator, and retrieve the number of processes.
 MPI_Comm_size (MPI_COMM_WORLD, &num_procs);
 // Determine the rank of the process.
 MPI_Comm_rank (MPI_COMM_WORLD, &id);
 // Get machine name
 MPI_Get_processor_name (name, &namelen);

 if (id == MASTER)
 {
 printf ("Hello world asd : rank %d of %d running on %s\n", id, num_procs, name);
 for (i = 1; i<num_procs; i++)
{
 MPI_Recv (&remote_id, 1, MPI_INT, i, TAG_HELLO, MPI_COMM_WORLD, &stat);
 MPI_Recv (&num_procs, 1, MPI_INT, i, TAG_HELLO, MPI_COMM_WORLD, &stat);
 MPI_Recv (&namelen, 1, MPI_INT, i, TAG_HELLO, MPI_COMM_WORLD, &stat);
 MPI_Recv (name, namelen+1, MPI_CHAR, i, TAG_HELLO, MPI_COMM_WORLD,
&stat);
 printf ("Hello world Burdaym: rank %d of %d running on %s\n", remote_id,
num_procs, name);
}
}
 else
 {
 MPI_Send (&id, 1, MPI_INT, MASTER, TAG_HELLO, MPI_COMM_WORLD);
 MPI_Send (&num_procs, 1, MPI_INT, MASTER, TAG_HELLO, MPI_COMM_WORLD);
 MPI_Send (&namelen, 1, MPI_INT, MASTER, TAG_HELLO, MPI_COMM_WORLD);
 MPI_Send (name, namelen+1, MPI_CHAR, MASTER, TAG_HELLO, MPI_COMM_WORLD);
}
 // Rank 0 distributes seek randomly to all processes.
 double startprocess, endprocess;
 int distributed_seed = 0;
 int *buff;
 buff = (int *)malloc(num_procs * sizeof(int));
 unsigned int MAX_NUM_POINTS = pow (2,32) - 1;
 unsigned int num_local_points = MAX_NUM_POINTS / num_procs;
 if (id == MASTER)
 {
 srand (time(NULL));

 for (i=0; i<num_procs; i++)
{
 distributed_seed = rand();
 buff[i] = distributed_seed;
}
 }


MPI_Bcast(buff, num_procs, MPI_INT, MASTER, MPI_COMM_WORLD);
 // At this point, every process (including rank 0) has a different seed. Using
//their seed,
 // each process generates N points randomly in the interval [1/n, 1, 1]
 startprocess = MPI_Wtime();
 srand (buff[id]);
 unsigned int point = 0;
 unsigned int rand_MAX = 128000;
 float p_x, p_y, p_z;
float temp, temp2, pi;
 double result;
 unsigned int inside = 0, total_inside = 0;
 for (point=0; point<num_local_points; point++)
 {
 temp = (rand() % (rand_MAX+1));
 p_x = temp / rand_MAX;
 p_x = p_x / num_procs;

 temp2 = (float)id / num_procs; // id belongs to 0, num_procs-1
 p_x += temp2;

 temp = (rand() % (rand_MAX+1));
 p_y = temp / rand_MAX;

 temp = (rand() % (rand_MAX+1));
 p_z = temp / rand_MAX;
 // Compute the number of points residing inside of the 1/8 of the sphere
 result = p_x * p_x + p_y * p_y + p_z * p_z;
 if (result <= 1)
 {
inside++;
 }
 }
 double elapsed = MPI_Wtime() - startprocess;
 MPI_Reduce (&inside, &total_inside, 1, MPI_UNSIGNED, MPI_SUM, MASTER,
MPI_COMM_WORLD);
#if DEBUG
 printf ("rank %d counts %u points inside the sphere\n", id, inside);
#endif
 if (id == MASTER)
 {
 double timeprocess[num_procs];
 timeprocess[MASTER] = elapsed;
 printf("Elapsed time from rank %d: %10.2f (sec) \n", MASTER,
timeprocess[MASTER]);

 for (i=1; i<num_procs; i++)
{
 // Rank 0 waits for elapsed time value
MPI_Recv (&timeprocess[i], 1, MPI_DOUBLE, i, TAG_TIME, MPI_COMM_WORLD,&stat);
 printf("Elapsed time from rank %d: %10.2f (sec)\n", i, timeprocess[i]);
}
 temp = 6 * (float)total_inside;
 pi = temp / MAX_NUM_POINTS;
 printf ( "Out of %u points, there are %u points inside the sphere =>pi=%16.12f\n", MAX_NUM_POINTS, total_inside, pi);
 }
 else
 {
 // Send back the processing time (in second)
 MPI_Isend (&elapsed, 1, MPI_DOUBLE, MASTER, TAG_TIME, MPI_COMM_WORLD);
 }
 free(buff);
 // Terminate MPI.
 MPI_Finalize();

 return 0;
}

