#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

void Dump::initAnalysisDump(Comm &comm, char *configFile){

	int i, retval;

	if (num_steps == 0)
		perror("initAnalysisDump: error in initializing");

//read from config file
	if (comm.me == 0) {

		FILE *fp = fopen (configFile, "r");

 		if (fp == NULL) {
			printf("Config file open error %d %s\n", errno, strerror(errno));
			exit(1);
		}	
		
		fscanf(fp, "%d", &anum);
		aalloc(anum);

		i = 0;
		while(i<anum) {
			fscanf(fp, "%d %s", &afreq[i], afname+i*FILENAMELEN);
			++i;
		}
	
		if (i != anum) {
			printf("Config file mistmatch %d %d\n", anum, i);
			exit(1);
		}

		fclose(fp);
	}

	MPI_Bcast(&anum, 1, MPI_INT, 0, comm.subcomm);

	//allocate anum elements in all processes but 0 (reader)
	if(afreq == NULL) aalloc(anum);

	MPI_Bcast(afreq, anum, MPI_INT, 0, comm.subcomm);
	if (MPI_Bcast(afname, anum*FILENAMELEN, MPI_CHAR, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis dump file name bcast error %d %s\n", errno, strerror(errno));

#ifdef DEBUG
	for (i=0; i<anum; i++) 
		printf("%d %d check %d %s len %d\n", comm.me, i, afreq[i], afname+i*FILENAMELEN, strlen(afname+i*FILENAMELEN));
#endif
	

	//open files
	afh = (MPI_File *) malloc(anum * sizeof(MPI_File)); 
	for (i=0; i<anum; i++) 
		retval = MPI_File_open (comm.subcomm, afname+i*FILENAMELEN, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &afh[i]);
	
	if (retval != MPI_SUCCESS) 
		printf("\nAnalysis dump file open error %d %s\n", errno, strerror(errno));

}

void Dump::apack(Atom &atom, Comm &comm, int n, int aindex) {

	int ret;
	nlocal = atom.nlocal;
	bufsize = nlocal;

	if (aindex < 2)
		arraylen = 3 * bufsize;
	else
		arraylen = bufsize;

	array = (MMD_float *) malloc (arraylen * sizeof(MMD_float));
	if (array == NULL)
		printf("Error: Memory allocation failed");
	

}

void Dump::adump(Atom &atom, Comm &comm, int n, int aindex) {

}

void Dump::aunpack() {

	delete array;
}

void Dump::writeAOutput(Atom &atom, Comm &comm, int n, int aindex) {
	
	apack(atom, comm, n, aindex);
	adump(atom, comm, n, aindex);
	aunpack(); 

}

void Dump::finiAnalysisDump() {

	//close files. cleanup.
	for (int i=0; i<anum; i++) {
		MPI_File_close (&afh[i]);
	}

	delete afname, afreq;
	free(afh);

}

void Dump::aalloc(int anum)
{
		afreq = new int[anum];
		afname = new char[anum*FILENAMELEN];
/*
		afname = new char*[anum];
		for (int j=0; j<anum; j++)
			afname[j] = new char[FILENAMELEN];
*/
}
