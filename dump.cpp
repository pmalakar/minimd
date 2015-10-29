#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

char dumpfile[] = "dump.txt";
char *posfile = "positions.txt";
char *velfile = "velocities.txt";
FILE *dumpfp;

int numAtoms;
int count, rcount;
float *pos, *vel, *rtest;

MPI_Offset mpifo;
MPI_File posfh, velfh;

void initDump(Comm &comm) {

	if (comm.me == 0) {
		dumpfp = fopen (dumpfile, "w");
 		if (dumpfp == NULL) {
			printf("File open error %d %s\n", errno, strerror(errno));
			exit(1);
		}
		printf("TESTING %d\n", PAD);
	}

	MPI_File_open(MPI_COMM_WORLD, posfile, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &posfh);

}

void pack(Atom &atom, int n, Comm &comm) {
/*
	  if (n == 1 || n == 100) printf("%d: CHECK BOX: %d %lf %lf %lf %lf %lf %lf\n", comm.me, n, atom.box.xlo, atom.box.xhi, atom.box.ylo, atom.box.yhi, atom.box.zlo, atom.box.zhi);
 * e.g. output
 * 		100: CHECK BOX: 1 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
		100: CHECK BOX: 100 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
 *
 */ 
		int ret;
	  double time;

		//float *pos = new float[3*atom.nlocal];
		//float *vel = new float[3*atom.nlocal];
		//float *rtest = new float[3*atom.nlocal];

		pos = (float *) malloc (3*atom.nlocal * sizeof(float));
		vel = (float *) malloc (3*atom.nlocal * sizeof(float));
		rtest = (float *) malloc (3*atom.nlocal * sizeof(float));

	  MPI_Scan(&atom.nlocal,&numAtoms,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    printf("%d: Mine %d Total atoms %d at %dth step\n", comm.me, atom.nlocal, numAtoms, n); 
	
	  double t = MPI_Wtime();

	  for(int i = 0; i < atom.nlocal ; i++) {
				pos[i * PAD + 0] = atom.x[i * PAD + 0], pos[i * PAD + 1] = atom.x[i * PAD + 1], pos[i * PAD + 2] = atom.x[i * PAD + 2];
				vel[i * PAD + 0] = atom.x[i * PAD + 0], vel[i * PAD + 1] = atom.x[i * PAD + 1], vel[i * PAD + 2] = atom.x[i * PAD + 2];
//    	fprintf(dumpfp, "%d: %d: atom %d of %d positions %lf %lf %lf\n", comm.me, n, i, atom.nlocal, atom.x[i * PAD + 0], atom.x[i * PAD + 1], atom.x[i * PAD + 2]);
    	if(dumpfp != NULL) {
				ret = fprintf(dumpfp, "%d: %d: atom %d of %d positions %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.x[i * PAD + 0], i * PAD + 1, atom.x[i * PAD + 1], i * PAD + 2, atom.x[i * PAD + 2]);
				if (ret < 0) {
					printf("fprintf error %d %s\n", errno, strerror(errno));
					if (ferror (dumpfp))
      				printf ("Error Writing to dump.txt\n");
				}
//    	fprintf(dumpfp, "%d: %d: atom %d of %d velocities %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.v[i * PAD + 0], i * PAD + 1, atom.v[i * PAD + 1], i * PAD + 2, atom.v[i * PAD + 2]);
			}
	  }

	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

}

void unpack(){

	delete pos, vel, rtest;
}

void dump(Atom &atom, int n, Comm &comm) {

		MPI_Offset offset;
		MPI_Status status;
		double time;
	  double t = MPI_Wtime();
	  
  	if (comm.me == 0)
    	MPI_File_get_size(posfh, &mpifo);
   	MPI_Bcast(&mpifo, 1, MPI_INT, 0, MPI_COMM_WORLD);

		offset = mpifo;
		mpifo += 3*numAtoms * sizeof(float);
		printf("%d: %d: Current offset %d %d\n", comm.me, n, mpifo, offset);
		
		MPI_File_set_view(posfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
		MPI_File_write_all(posfh, pos, 3*atom.nlocal, MPI_FLOAT, &status);
		MPI_Get_count (&status, MPI_FLOAT, &count);
		printf("%d: %d: written %d entries %f %f\n", comm.me, n, count, atom.x[0], atom.x[1]);

			//MPI_File_seek(posfh, mpifo, MPI_SEEK_SET);
		MPI_File_set_view(posfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
		MPI_File_read_all(posfh, rtest, 3*atom.nlocal, MPI_FLOAT, &status);
		MPI_Get_count (&status, MPI_FLOAT, &rcount);
		printf("%d: %d: read %d floats\n", comm.me, n, rcount);
		if (n < 3)
	 		for(int i = 0; i < atom.nlocal ; i++) 
				printf("%d: %d: read %f %f %f\n", comm.me, n, rtest[i*PAD+0], rtest[i*PAD+1], rtest[i*PAD+2]);

	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

}

void writeFile(Atom &atom, int n, Comm &comm) {
	
	pack(atom, n, comm);
	dump(atom, n, comm);
	unpack(); //atom, n, comm);

}

void finiDump(Comm &comm) {

	if (comm.me == 0) 
		fclose(dumpfp);
	MPI_File_close(&posfh);

}


