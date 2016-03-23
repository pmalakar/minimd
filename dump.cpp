#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

Dump::Dump(){

	dumpfile = new char[10];
	posfile = new char[256];
	posfilename = new char[64];
	velfile = new char[256];
	velfilename = new char[64];

	strcpy(dumpfile, "dump.txt");
	strcpy(posfilename, "positions.txt");
	strcpy(velfilename, "velocities.txt");

	bufsize = 0;
}

Dump::~Dump() {}

void Dump::initDump(Comm &comm, int ts, int dfreq, char *dumpdir) {
	

	if (comm.me == 0) {
		dumpfp = fopen (dumpfile, "w");
 		if (dumpfp == NULL) {
			printf("File open error %d %s\n", errno, strerror(errno));
			exit(1);
		}
		//printf("TESTING %d\n", PAD); PAD = 3
		printf("TESTING %d %d %d\n", sizeof(MPI_DOUBLE), sizeof(double), sizeof(MMD_float));
	}

	if (sizeof(MMD_float) == 4)
		dtype = MPI_FLOAT;
	else if (sizeof(MMD_float) == 8)
		dtype = MPI_DOUBLE;

	num_steps = ts;
	output_frequency = dfreq;

	if (dumpdir != NULL) {
		printf("%d %s\n",comm.me, dumpdir);
		strcpy(posfile, dumpdir);
		strcat(posfile, "/");
		strcat(posfile, posfilename);
		strcpy(velfile, dumpdir);
		strcat(velfile, "/");
		strcat(velfile, velfilename);
	}
	else {
		strcpy(posfile, posfilename);
		strcpy(velfile, velfilename);
		printf("%s %s\n",posfilename, velfilename);
	}
	printf("%s %s\n",posfile, velfile);

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
	nlocal = atom.nlocal;

//	if (!pos || bufsize < atom.nlocal) {
		bufsize = nlocal; 
		pos = (MMD_float *) malloc (3*bufsize * sizeof(MMD_float));
		vel = (MMD_float *) malloc (3*bufsize * sizeof(MMD_float));
		rtest = (MMD_float *) malloc (3*bufsize * sizeof(MMD_float));
//	}

	//nlocal - local number of atoms in the current rank
	//totalAtoms - total number of atoms in the system
	
	MPI_Scan(&nlocal, &numAtoms, 1, MPI_LONG_LONG_INT, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce (&numAtoms, &totalAtoms, 1, MPI_LONG_LONG_INT, MPI_MAX, MPI_COMM_WORLD);
  if(comm.me == 0 || (comm.me < 3 && n < 3)) 
		printf("%d: %d: Mine %lld Partial sum %lld Total atoms %lld\n", comm.me, n, nlocal, numAtoms, totalAtoms); 

	//  double t = MPI_Wtime();

	//copy from simulation buffer
	for(int i = 0; i < nlocal ; i++) {

		//positions
		pos[i * PAD + 0] = atom.x[i * PAD + 0], pos[i * PAD + 1] = atom.x[i * PAD + 1], pos[i * PAD + 2] = atom.x[i * PAD + 2];
		//velocities
		vel[i * PAD + 0] = atom.v[i * PAD + 0], vel[i * PAD + 1] = atom.v[i * PAD + 1], vel[i * PAD + 2] = atom.v[i * PAD + 2];

	}

	//  t = MPI_Wtime() - t;
	//  MPI_Allreduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

}

void Dump::unpack(){

	delete pos, vel, rtest;
}

void Dump::dump(Atom &atom, int n, Comm &comm) {

	MPI_Status status;
	double time;

// 	if (comm.me == 0) {
//    	MPI_File_get_size(posfh, &mpifo);
//    	MPI_File_get_size(velfh, &mpifo);
//	}
   	//MPI_Bcast(&mpifo, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	mpifo = 3 * (n*totalAtoms + numAtoms - nlocal) * sizeof(MMD_float);

	if (comm.me == 0 || (comm.me != 0 && n < 3)) 
		printf("%d: %d: Current offset %lld | %lld %lld %lld\n", comm.me, n, mpifo, totalAtoms, numAtoms, nlocal);

	double t = MPI_Wtime();

	//MPI_File_set_view(posfh, mpifo, MPI_FLOAT, MPI_FLOAT, "native", MPI_INFO_NULL);
	//MPI_File_write_all(posfh, pos, 3*atom.nlocal, MPI_FLOAT, &status);

	if (MPI_File_write_at_all(posfh, mpifo, pos, 3*nlocal, dtype, &status) != MPI_SUCCESS) 
		perror("Positions write unsuccessful");
	if (MPI_File_write_at_all(velfh, mpifo, vel, 3*nlocal, dtype, &status) != MPI_SUCCESS) 
		perror("Velocities write unsuccessful");

	t = MPI_Wtime() - t;
	MPI_Get_count (&status, dtype, &count);

	MPI_Allreduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	if (comm.me == 0 && n<5) {
		printf("%d: %d: written %d doubles (offset %d) in %4.2lf s\n", comm.me, n, count, mpifo, time);
		printf("%d: %d: written %d entries %lf %lf %lf\n", comm.me, n, count, pos[0], pos[1], pos[2]);
		printf("%d: %d: written %d velocities %lf %lf %lf\n", comm.me, n, count, vel[0], vel[1], vel[2]);
	}

#ifdef DEBUG
	if (n < 3 && comm.me < 4)	
		for(int i = 0; i < nlocal ; i++) 
			printf("%d: %d: wrote %dth atom %lf %lf %lf\n", comm.me, n, i, pos[i*PAD+0], pos[i*PAD+1], pos[i*PAD+2]);
#endif

//verify
	MPI_File_read_at_all(velfh, mpifo, rtest, 3*nlocal, dtype, &status);
	MPI_Get_count (&status, dtype, &rcount);
	if (comm.me == 0) printf("%d: %d: have read %d doubles\n", comm.me, n, rcount);

#ifdef DEBUG
	if (n < 3 && comm.me < 3)
		for(int i = 0; i < nlocal ; i++) 
			printf("%d: %d: read %dth atom %lf %lf %lf\n", comm.me, n, i, rtest[i*PAD+0], rtest[i*PAD+1], rtest[i*PAD+2]);
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


