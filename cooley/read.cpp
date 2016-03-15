#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mpi.h>

char *posfile = "positions.txt";
char *velfile = "velocities.txt";
FILE *dumpfp;
int PAD = 3;

int me, nprocs;
int numAtoms, totalAtoms;
int rcount;
float *pos, *vel, *rtest;

MPI_Offset mpifo;
MPI_File posfh, velfh;

void read() {

		MPI_Offset offset;
		MPI_Status status;
		double time;
		int atomPerProc, timesteps;
	  
		//4000000 atom positions written from 255 processes 
		totalAtoms = 4000000;
		timesteps = 20;
		atomPerProc = totalAtoms/nprocs;
		pos = (float *) malloc (3 * atomPerProc * sizeof(float));
		
		int n = 0;
		MPI_File_open(MPI_COMM_WORLD, posfile, MPI_MODE_RDONLY, MPI_INFO_NULL, &posfh);
		MPI_File_read_all(posfh, pos, 3*atomPerProc, MPI_FLOAT, &status);
		MPI_File_close(&posfh);

		MPI_Get_count (&status, MPI_FLOAT, &rcount);
		if (me < 2) printf("%d: have read %d floats\n", me, rcount);

		if (n < 3 && me < 3)
	 		for(int i = 0; i < 3 ; i++) 
				printf("%d: %d: read %dth atom %f %f %f\n", me, n, i, pos[i*PAD+0], pos[i*PAD+1], pos[i*PAD+2]);

}

int main (int argc, char** argv) {

	MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &me);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

	read();

	MPI_Finalize();
	return 0;

}

