#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

char dumpfile[] = "dump.txt";
char *filename = "fdump.txt";
FILE *dumpfp;
MPI_Offset mpifo;
MPI_File mpifh;

void initDump(Comm &comm) {

	if (comm.me == 0) {
		dumpfp = fopen (dumpfile, "w");
 		if (dumpfp == NULL) {
			printf("File open error %d %s\n", errno, strerror(errno));
			exit(1);
		}
		printf("TESTING %d\n", PAD);
	}

	MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &mpifh);

}

void dump(Atom &atom, int n, Comm &comm) {

/*
	  if (n == 1 || n == 100) printf("%d: CHECK BOX: %d %lf %lf %lf %lf %lf %lf\n", comm.me, n, atom.box.xlo, atom.box.xhi, atom.box.ylo, atom.box.yhi, atom.box.zlo, atom.box.zhi);
 * e.g. output
 * 		100: CHECK BOX: 1 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
		100: CHECK BOX: 100 2.099495 4.198990 8.397981 10.497476 8.397981 10.497476
 *
 */ 
		int ret;
	  int numAtoms;
	  double time;
		MPI_Offset offset;
		MPI_Status status;
		int count, rcount;

	  MPI_Scan(&atom.nlocal,&numAtoms,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    printf("%d: Mine %d Total atoms %d at %dth step\n", comm.me, atom.nlocal, numAtoms, n); 
	
	  double t = MPI_Wtime();

		if (comm.me == 0) {
	  for(int i = 0; i < atom.nlocal ; i++) {
//    	fprintf(dumpfp, "%d: %d: atom %d of %d positions %lf %lf %lf\n", comm.me, n, i, atom.nlocal, atom.x[i * PAD + 0], atom.x[i * PAD + 1], atom.x[i * PAD + 2]);
    	//printf("%d: %d: atom %d of %d positions %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.x[i * PAD + 0], i * PAD + 1, atom.x[i * PAD + 1], i * PAD + 2, atom.x[i * PAD + 2]);
    	if(dumpfp != NULL) {
				ret = fprintf(dumpfp, "%d: %d: atom %d of %d positions %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.x[i * PAD + 0], i * PAD + 1, atom.x[i * PAD + 1], i * PAD + 2, atom.x[i * PAD + 2]);
				if (ret < 0) {
					printf("fprintf error %d %s\n", errno, strerror(errno));
					if (ferror (dumpfp))
      				printf ("Error Writing to dump.txt\n");
				}
				//else {
				//	printf("fprintf returns %d\n", ret);
				//}
			}
//    	printf("%d: %d: atom %d of %d velocities %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.v[i * PAD + 0], i * PAD + 1, atom.v[i * PAD + 1], i * PAD + 2, atom.v[i * PAD + 2]);
	  }
		}

	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

	  t = MPI_Wtime();
	  
  	if (comm.me == 0)
    	MPI_File_get_size(mpifh, &mpifo);
   	MPI_Bcast(&mpifo, 1, MPI_INT, 0, MPI_COMM_WORLD);

		offset = mpifo;
		mpifo += 3*numAtoms * sizeof(float);
		printf("%d: %d: Current offset %d %d\n", comm.me, n, mpifo, offset);
		
		float *test = new float[3*atom.nlocal];
		float *rtest = new float[3*atom.nlocal];
		test[0] = comm.me + 2;
		test[1] = comm.me - 2;
		test[2] = comm.me ;

	  if (n == 0) {
			MPI_File_set_view(mpifh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
			MPI_File_write_all(mpifh, test, 3*atom.nlocal, MPI_FLOAT, &status);
			//MPI_File_write_all(mpifh, atom.x, 3*atom.nlocal, MPI_FLOAT, &status);
			//MPI_File_write_at_all(mpifh, mpifo, atom.x, 3*atom.nlocal, MPI_FLOAT, &status);
			MPI_Get_count (&status, MPI_FLOAT, &count);
			printf("%d: %d: written %d entries %f %f\n", comm.me, n, count, atom.x[0], atom.x[1]);

			//MPI_File_seek(mpifh, mpifo, MPI_SEEK_SET);
			MPI_File_set_view(mpifh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
			MPI_File_read_all(mpifh, rtest, 3*atom.nlocal, MPI_FLOAT, &status);
			MPI_Get_count (&status, MPI_FLOAT, &rcount);
			printf("%d: %d: read %d floats %f %f %f\n", comm.me, n, rcount, rtest[0], rtest[1], rtest[2]);
		}	

	  t = MPI_Wtime() - t;
	  MPI_Reduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

		delete test;
}

void finiDump() {

	fclose(dumpfp);
	MPI_File_close(&mpifh);

}


