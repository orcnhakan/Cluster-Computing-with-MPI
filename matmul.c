/******************************************************************************
* FILE: mpi_mm.c
* DESCRIPTION:  
*   MPI Matrix Multiply - C Version
*   In this code, the master task distributes a matrix multiply
*   operation to numtasks-1 worker tasks.
*   NOTE:  C and Fortran versions of this code differ because of the way
*   arrays are stored/passed.  C arrays are row-major order but Fortran
*   arrays are column-major order.
* AUTHOR: Blaise Barney. Adapted from Ros Leibensperger, Cornell Theory
*   Center. Converted to MPI: George L. Gusciora, MHPCC (1/95)
* LAST REVISED: 04/13/05
******************************************************************************/
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#define NRA 4                  /* A matrisinin satır sayısı */
#define NCA 4                 /* A matrisinin sütun sayısı */
#define NCB 4                 /* B matrisinin sütun sayısı */
#define MASTER 0               /* İlk görevin(task) taskid'si */
#define FROM_MASTER 1          /* Mesaj tipini ayarlama */
#define FROM_WORKER 2          /* Mesaj tipini ayarlama */
#define TAG_HELLO 4

// 1. demo için 4'e 4 lük a ve b matrislerini dolduruyor.
// NRA, NCA, NCB değişkenleri 4 yapmayı unutmayın.
void demo_1_fillMatrix(double a[NRA][NCA], double b[NCA][NCB]) {
	a[0][0] = 1;
	a[0][1] = 3;
	a[0][2] = -2;
	a[0][3] = 4;
	a[1][0] = 5;
	a[1][1] = 2;
	a[1][2] = -4;
	a[1][3] = 5;
	a[2][0] = -4;
	a[2][1] = 11;
	a[2][2] = 7;
	a[2][3] = 5;
	a[3][0] = -3;
	a[3][1] = 3;
	a[3][2] = 4;
	a[3][3] = 9;

	b[0][0] = -4;
	b[0][1] = 12;
	b[0][2] = 5;
	b[0][3] = 7;
	b[1][0] = 1;
	b[1][1] = 1;
	b[1][2] = 7;
	b[1][3] = 9;
	b[2][0] = -3;
	b[2][1] = -4;
	b[2][2] = -7;
	b[2][3] = 5;
	b[3][0] = 1;
	b[3][1] = 6;
	b[3][2] = 3;
	b[3][3] = 2;

}

void demo_2_fillMatrix(double a[NRA][NCA], double b[NCA][NCB]) {
      int i, j;
      for (i=0; i<NRA; i++)
         for (j=0; j<NCA; j++)
            a[i][j]= i+j;
      for (i=0; i<NCA; i++)
         for (j=0; j<NCB; j++)
            b[i][j]= i*j;
}


int main (int argc, char *argv[])
{
int	numtasks,              /* number of tasks in partition */
	taskid,                /* Görev(task) tanımlayıcı*/
	numworkers,            /* number of worker tasks */
	source,                /* task id of message source */
	dest,                  /* task id of message destination */
	mtype,                 /* message type */
	rows,                  /* rows of matrix A sent to each worker */
	averow, extra, offset, /* used to determine rows sent to each worker */
	i, j, k, rc;           /* misc */
double	a[NRA][NCA],           /* matrix A to be multiplied */
	b[NCA][NCB],           /* matrix B to be multiplied */
	c[NRA][NCB];           /* result matrix C */

int namelen;
MPI_Status status;

MPI_Init(&argc,&argv);
MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
if (numtasks < 2 ) {
  printf("Need at least two MPI tasks. Quitting...\n");
  MPI_Abort(MPI_COMM_WORLD, rc);
  exit(1);
  }
numworkers = numtasks-1;

char name[MPI_MAX_PROCESSOR_NAME];
MPI_Get_processor_name (name, &namelen);
/**************************** master task ************************************/
   if (taskid == MASTER)
   {
      printf("mpi_mm has started with %d tasks.\n",numtasks);
      printf("Initializing arrays...\n");

      demo_1_fillMatrix(a, b);
      // demo_2_fillMatrix(a, b);
      /* Send matrix data to the worker tasks */
      printf("Num of workers: %d\n", numworkers);
      averow = NRA/numworkers;
      extra = NRA%numworkers;
      offset = 0;
      mtype = FROM_MASTER;
      for (dest=1; dest<=numworkers; dest++)
      {
         rows = (dest <= extra) ? averow+1 : averow;   	
         printf("Sending %d rows to task %d to machine %s - offset=%d\n", rows, dest, name, offset);
         MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
         MPI_Send(&a[offset][0], rows*NCA, MPI_DOUBLE, dest, mtype,
                   MPI_COMM_WORLD);
         MPI_Send(&b, NCA*NCB, MPI_DOUBLE, dest, mtype, MPI_COMM_WORLD);
         offset = offset + rows;
      }

      /* Receive results from worker tasks */
      mtype = FROM_WORKER;
      for (i=1; i<=numworkers; i++)
      {
         source = i;
         MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
         MPI_Recv(&c[offset][0], rows*NCB, MPI_DOUBLE, source, mtype, 
                  MPI_COMM_WORLD, &status);
         printf("Received results from task %d\n",source);
      }

      /* Print results */
      printf("******************************************************\n");
      printf("Result Matrix:\n");
     
for (i=0; i<NRA; i++)
      {
         printf("\n"); 
         for (j=0; j<NCB; j++) 
            printf("%6.2f   ", c[i][j]);
      }
      printf("\n******************************************************\n");
      printf ("Done.\n");

   }


/**************************** worker task ************************************/
   if (taskid > MASTER)
   {
      mtype = FROM_MASTER;
      MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&a, rows*NCA, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);
      MPI_Recv(&b, NCA*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD, &status);

      for (k=0; k<NCB; k++)
         for (i=0; i<rows; i++)
         {
            c[i][k] = 0.0;
            for (j=0; j<NCA; j++)
               c[i][k] = c[i][k] + a[i][j] * b[j][k];
         }
      mtype = FROM_WORKER;
      MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
      MPI_Send(&c, rows*NCB, MPI_DOUBLE, MASTER, mtype, MPI_COMM_WORLD);
   }
   MPI_Finalize();
}
