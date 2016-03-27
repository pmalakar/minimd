#ifndef DUMP_H_
#define DUMP_H_

#include <stdint.h>
#include "mpi.h"
#include "atom.h"
#include "comm.h"

#define FILENAMELEN 64

class Dump 
{

	char *dumpfile; //[] = "dump.txt";
	char *posfile, *posfilename; // = "positions.txt";
	char *velfile, *velfilename; // = "velocities.txt";
	//FILE *dumpfp, *configfp;

	int output_frequency;
	int num_steps;

	long long int nlocal;
	long long int numAtoms;
	long long int totalAtoms;
	long long int bufsize;
	int count, rcount;

	MPI_Offset mpifo;
	MPI_File posfh, velfh;
	
	MPI_Datatype dtype;

	int anum;
	int *afreq;
	char **afname;

	double *time_to_write;

  public:

  Dump();
  ~Dump();

	int getFreq();
  void initDump(Comm &, int, int, char *, char *);
	void writeFile(Atom &, int, Comm &);
	void pack(Atom &, int, Comm &);
	void dump(Atom &, int, Comm &);
	void unpack(void);
	void finiDump(Comm &);

	void initAnalysisDump(Comm &, char *);
	void finiAnalysisDump(Comm &);

	MMD_float *pos, *vel, *rtest;
	char *dumpdir;
	
};

#endif

