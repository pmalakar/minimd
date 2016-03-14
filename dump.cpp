#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

Dump::Dump(){

	dumpfile = new char[10];
	posfile = new char[20];
	velfile = new char[20];
	strcpy(dumpfile, "dump.txt");
	strcpy(posfile, "positions.txt");
	strcpy(velfile, "velocities.txt");

	bufsize = 0;
}

Dump::~Dump() {}

void Dump::initDump(Comm &comm, int ts, int dfreq) {
	
	if (comm.me == 0) {
		dumpfp = fopen (dumpfile, "w");
 		if (dumpfp == NULL) {
			printf("File open error %d %s\n", errno, strerror(errno));
			exit(1);
		}
		//printf("TESTING %d\n", PAD); PAD = 3
	}

	num_steps = ts;
	output_frequency = dfreq;

	if (output_frequency > ts)
		perror("Output frequency cannot be > total number of time steps");

	MPI_File_open (MPI_COMM_WORLD, posfile, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &posfh);
	MPI_File_open (MPI_COMM_WORLD, velfile, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &velfh);
}

int Dump::getFreq() {

	return output_frequency;
}

void Dump::pack(Atom &atom, int n, Comm &comm) {
 
	int ret;

//	if (!pos || bufsize < atom.nlocal) {
		bufsize = atom.nlocal; // * 1.2 ;	
		pos = (float *) malloc (3*bufsize * sizeof(float));
		//pos = (float *) malloc (3*atom.nlocal * sizeof(float));
		vel = (float *) malloc (3*bufsize * sizeof(float));
		rtest = (float *) malloc (3*bufsize * sizeof(float));
//	}

	MPI_Scan(&atom.nlocal, &numAtoms, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce (&numAtoms, &totalAtoms, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    printf("%d: %d: Mine %d Partial sum %d Total atoms %d\n", comm.me, n, atom.nlocal, numAtoms, totalAtoms); 

	//  double t = MPI_Wtime();

	//copy from simulation buffer
	for(int i = 0; i < atom.nlocal ; i++) {

		//positions
		pos[i * PAD + 0] = atom.x[i * PAD + 0], pos[i * PAD + 1] = atom.x[i * PAD + 1], pos[i * PAD + 2] = atom.x[i * PAD + 2];
		//velocities
		vel[i * PAD + 0] = atom.v[i * PAD + 0], vel[i * PAD + 1] = atom.v[i * PAD + 1], vel[i * PAD + 2] = atom.v[i * PAD + 2];


//    	fprintf(dumpfp, "%d: %d: atom %d of %d positions %lf %lf %lf\n", comm.me, n, i, atom.nlocal, atom.x[i * PAD + 0], atom.x[i * PAD + 1], atom.x[i * PAD + 2]);
    	//if(dumpfp != NULL) {
		//		ret = fprintf(dumpfp, "%d: %d: atom %d of %d positions %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.x[i * PAD + 0], i * PAD + 1, atom.x[i * PAD + 1], i * PAD + 2, atom.x[i * PAD + 2]);
		//		if (ret < 0) {
		//			printf("fprintf error %d %s\n", errno, strerror(errno));
		//			if (ferror (dumpfp))
      	//			printf ("Error Writing to dump.txt\n");
		//		}
//    	fprintf(dumpfp, "%d: %d: atom %d of %d velocities %d %lf | %d %lf | %d %lf\n", comm.me, n, i, atom.nlocal, i * PAD + 0, atom.v[i * PAD + 0], i * PAD + 1, atom.v[i * PAD + 1], i * PAD + 2, atom.v[i * PAD + 2]);
		//	}
	}

	//  t = MPI_Wtime() - t;
	//  MPI_Allreduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

}

void Dump::unpack(){

	delete pos, vel, rtest;
}

void Dump::dump(Atom &atom, int n, Comm &comm) {

	MPI_Offset offset;
	MPI_Status status;
	double time;

  	if (comm.me == 0) {
    	MPI_File_get_size(posfh, &mpifo);
//    	MPI_File_get_size(velfh, &mpifo);
	}

   	MPI_Bcast(&mpifo, 1, MPI_INT, 0, MPI_COMM_WORLD);

	offset = mpifo;
	mpifo += 3*numAtoms * sizeof(float);
	if (comm.me == 0) printf("%d: %d: Current offset %d %d\n", comm.me, n, mpifo, offset);

	double t = MPI_Wtime();
	//MPI_File_set_view(posfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
	//MPI_File_write_all(posfh, pos, 3*atom.nlocal, MPI_FLOAT, &status);
	MPI_File_write_at_all(posfh, mpifo, pos, 3*atom.nlocal, MPI_FLOAT, &status);
	//MPI_File_set_view(velfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
	//MPI_File_write_all(velfh, vel, 3*atom.nlocal, MPI_FLOAT, &status);
	MPI_File_write_at_all(velfh, mpifo, vel, 3*atom.nlocal, MPI_FLOAT, &status);
	t = MPI_Wtime() - t;
	//		if (status != MPI_SUCCESS) perror("Collective write unsuccessful");
	MPI_Get_count (&status, MPI_FLOAT, &count);

	MPI_Allreduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	if (comm.me == 0) {
		printf("%d: %d: written %d floats (offset %d) in %4.2lf s\n", comm.me, n, count, mpifo, time);
		//printf("%d: %d: written %f %f %f\n", comm.me, n, atom.x[0], atom.x[1], atom.x[2]);
		printf("%d: %d: written %d entries %f %f %f\n", comm.me, n, count, pos[0], pos[1], pos[2]);
	}

	//#ifdef DEBUG
	if (n < 3 && comm.me < 4)	
		for(int i = 0; i < atom.nlocal ; i++) 
			printf("%d: %d: wrote %dth atom %f %f %f\n", comm.me, n, i, pos[i*PAD+0], pos[i*PAD+1], pos[i*PAD+2]);
//#endif

//verify
	//MPI_File_open(MPI_COMM_WORLD, posfile, MPI_MODE_RDONLY, MPI_INFO_NULL, &posfh);
	//MPI_File_set_view(posfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
	//MPI_File_read_all(posfh, rtest, 3*atom.nlocal, MPI_FLOAT, &status);
	MPI_File_read_at_all(posfh, mpifo, rtest, 3*atom.nlocal, MPI_FLOAT, &status);
	//MPI_File_close(&posfh);
	MPI_Get_count (&status, MPI_FLOAT, &rcount);
	if (comm.me == 0) printf("%d: %d: have read %d floats\n", comm.me, n, rcount);
#ifdef DEBUG
	if (n < 3 && comm.me < 4)
		for(int i = 0; i < atom.nlocal ; i++) 
			printf("%d: %d: read %dth atom %f %f %f\n", comm.me, n, i, rtest[i*PAD+0], rtest[i*PAD+1], rtest[i*PAD+2]);
#endif

}

void Dump::writeFile(Atom &atom, int n, Comm &comm) {
	
	pack(atom, n, comm);
	dump(atom, n, comm);
	unpack(); //atom, n, comm);

}

void Dump::finiDump(Comm &comm) {

	if (pos)
		delete pos, vel, rtest;

	if (comm.me == 0) 
		fclose(dumpfp);

	MPI_File_close(&posfh);
	MPI_File_close(&velfh);

}


