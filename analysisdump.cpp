/*
 * Developed at Argonne National Laboratory
 *
 * Contact: pmalakar@anl.gov, malakar.preeti@gmail.com
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "comm.h"
#include "atom.h"
#include "dump.h"

void Dump::initAnalysisDump(Comm &comm, char *analysiscfg){

	int i, retval;

	if (analysiscfg == NULL) { 
		printf("Config file NULL error %d\n", comm.me);
		exit(1);
	}

	if (num_steps == 0)
		perror("initAnalysisDump: error in initializing");

	configFile = new char[strlen(analysiscfg)];
	strcpy(configFile, analysiscfg);

  //read from config file
	if (comm.me == 0) {

		FILE *fp = fopen (analysiscfg, "r");

 		if (fp == NULL) {
			printf("Config file open error %d %s\n", errno, strerror(errno));
			exit(1);
		}	
		
		fscanf(fp, "%d", &anum);
		aalloc(anum);

		i = 0;
		while(i<anum) {
      fscanf(fp, "%d %d %d %d %s %s", &adim[i], &atevery[i], &atsteps[i], &acurrstep[i], afname+i*FILENAMELEN, aname+i*ANAMELEN);
			++i;
		}
	
		fclose(fp);
	}

	MPI_Bcast(&anum, 1, MPI_INT, 0, comm.subcomm);

	//allocate anum elements in all processes but 0 (reader)
	if(afreq == NULL) aalloc(anum);

	if (MPI_Bcast(adim, anum, MPI_INT, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis config adim bcast error %d %s\n", errno, strerror(errno));
	if (MPI_Bcast(atevery, anum, MPI_INT, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis config atevery bcast error %d %s\n", errno, strerror(errno));
	if (MPI_Bcast(atsteps, anum, MPI_INT, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis config atsteps bcast error %d %s\n", errno, strerror(errno));
	if (MPI_Bcast(acurrstep, anum, MPI_INT, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis config acurrstep bcast error %d %s\n", errno, strerror(errno));
	if (MPI_Bcast(afname, anum*FILENAMELEN, MPI_CHAR, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis dump file name bcast error %d %s\n", errno, strerror(errno));
	if (MPI_Bcast(aname, anum*ANAMELEN, MPI_CHAR, 0, comm.subcomm) != MPI_SUCCESS)
		printf("\nAnalysis name bcast error %d %s\n", errno, strerror(errno));

	for (i=0; i<anum; i++) { 
		afreq[i] = num_steps/atsteps[i];	//todo: deal with integrality later
		retval = MPI_File_open (comm.subcomm, afname+i*FILENAMELEN, MPI_MODE_RDWR | MPI_MODE_CREATE, MPI_INFO_NULL, &afh[i]);
//#ifdef DEBUG
		if (comm.me < 2)
			printf("%d %d check %d %s %s\n", comm.me, i, afreq[i], afname+i*FILENAMELEN, aname+i*ANAMELEN);
//#endif
	}

	if (retval != MPI_SUCCESS) 
		printf("\nAnalysis dump file open error %d %s\n", errno, strerror(errno));

}

void Dump::apack(Atom &atom, Comm &comm, int n, int aindex) {

	int ret;
	nlocal = atom.nlocal;
	bufsize = nlocal;

	/*
	if (aindex < 2)
		arraylen = 3 * bufsize;
	else
		arraylen = bufsize;*/

	arraylen = adim[aindex] * nlocal;
	array = (MMD_float *) malloc (arraylen * sizeof(MMD_float));
	if (array == NULL)
		printf("Error: Memory allocation failed");
	
	MPI_Scan(&nlocal, &numAtoms, 1, MPI_LONG_LONG_INT, MPI_SUM, comm.subcomm);
  MPI_Allreduce (&numAtoms, &totalAtoms, 1, MPI_LONG_LONG_INT, MPI_MAX, comm.subcomm);

//fix
  for(int i = 0; i < nlocal ; i++) {
		if (aindex == 0)
    	array[i * PAD + 0] = atom.x[i * PAD + 0], array[i * PAD + 1] = atom.x[i * PAD + 1], array[i * PAD + 2] = atom.x[i * PAD + 2]; 
		else if (aindex == 1)
			array[i * PAD + 0] = atom.v[i * PAD + 0], array[i * PAD + 1] = atom.v[i * PAD + 1], array[i * PAD + 2] = atom.v[i * PAD + 2];
		else if (aindex == 2) array[i] = atom.x[i * PAD + 0];
		else if (aindex == 3) array[i] = atom.x[i * PAD + 1];
		else if (aindex == 4) array[i] = atom.v[i * PAD + 0];
		else if (aindex == 5) array[i] = atom.v[i * PAD + 1];
  }

}

void Dump::adump(Atom &atom, Comm &comm, int n, int aindex) {

	MPI_Status status;
	//int multiplier;
  double time;

	//if (aindex < 2)	multiplier = 3;
	//else multiplier = 1;

  mpifo = adim[aindex] * (n*totalAtoms + numAtoms - nlocal) * sizeof(MMD_float);
	if (comm.me < 2) printf("%d: test %lld %lld %lld %lld\n", comm.me, nlocal, numAtoms, totalAtoms, arraylen);

  double t = MPI_Wtime();

  if (MPI_File_write_at_all(afh[aindex], mpifo, array, arraylen, dtype, &status) != MPI_SUCCESS) 
    perror("Analysis write unsuccessful");

  t = MPI_Wtime() - t;
  MPI_Allreduce (&t, &time, 1, MPI_DOUBLE, MPI_MAX, comm.subcomm);

  MPI_Get_count (&status, dtype, &count);

//  time_to_write[n] = time;
	if (comm.me == 0) printf("%d: %d: %d: written %d doubles (offset %lld) in %4.2lf s\n", comm.me, n, aindex, count, mpifo, time);

}

void Dump::aunpack() {

	delete array;
}

void Dump::updateConfig(Comm &comm) {

	//update acurrstep for aindex 	
	if (comm.me == 0) {
    FILE *fp = fopen (configFile, "w+");
    if (fp == NULL) {
      printf("Config file update error %d %s\n", errno, strerror(errno));
      exit(1);
		}

		fprintf(fp, "%d\n", anum);
    int i = 0;
    while(i<anum) {			
      fprintf(fp, "%d %d %d %d %s %s\n", adim[i], atevery[i], atsteps[i], acurrstep[i], afname+i*FILENAMELEN, aname+i*ANAMELEN);
      ++i;
    }
		fclose(fp);
	}
}

void Dump::writeAOutput(Atom &atom, Comm &comm, int n, int aindex) {
	
	apack(atom, comm, n, aindex);
	adump(atom, comm, n, aindex);
	aunpack(); 
	
	acurrstep[aindex] ++;
	updateConfig(comm);

}

void Dump::aalloc(int anum)
{
		adim = new int[anum];
		atevery = new int[anum];
		atsteps = new int[anum];
		acurrstep = new int[anum];
		afreq = new int[anum];
		afname = new char[anum*FILENAMELEN];
		aname = new char[anum*ANAMELEN];

		afh = (MPI_File *) malloc(anum * sizeof(MPI_File)); 
}


void Dump::finiAnalysisDump() {

	//close files. cleanup.
	for (int i=0; i<anum; i++) {
		MPI_File_close (&afh[i]);
	}

	delete adim;
	delete atevery;
	delete atsteps;
	delete acurrstep;
	delete afreq;
	delete afname;
	delete aname;
	free(afh);

}

