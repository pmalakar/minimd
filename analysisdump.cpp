#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

void Dump::initAnalysisDump(Comm &comm, char *configFile){

	int i;

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
			fscanf(fp, "%d %s", &afreq[i], afname[i]);
			++i;
		}
	
		if (i != anum) {
			printf("Config file mistmatch %d %d\n", anum, i);
			exit(1);
		}

		for (i=0; i<anum; i++)
			printf("check %d %s\n", afreq[i], afname[i]);

		fclose(fp);
	}

	MPI_Bcast(&anum, 1, MPI_INT, 0, comm.subcomm);
	if(afreq == NULL) aalloc(anum);

	//open files

}

void Dump::finiAnalysisDump(Comm &comm) {

	//cleanup
	//close files

}

void Dump::aalloc(int anum)
{
		afreq = new int[anum];
		afname = new char*[anum];
		for (int j=0; j<anum; j++)
			afname[j] = new char[FILENAMELEN];
}
