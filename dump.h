#ifndef DUMP_H_
#define DUMP_H_

#include <stdint.h>
#include "mpi.h"
#include "atom.h"
#include "comm.h"

class Dump 
{

	char *dumpfile; //[] = "dump.txt";
	char *posfile, *posfilename; // = "positions.txt";
	char *velfile, *velfilename; // = "velocities.txt";
	FILE *dumpfp;

	int output_frequency;
	int num_steps;

	int numAtoms;
	int totalAtoms;
	int bufsize;
	int count, rcount;

	int64_t mpifo;
	MPI_File posfh, velfh;

  public:
    Dump();
    ~Dump();
	int getFreq();
  void initDump(Comm &, int, int, char *);
	void writeFile(Atom &, int, Comm &);
	void pack(Atom &, int, Comm &);
	void dump(Atom &, int, Comm &);
	void unpack(void);
	void finiDump(Comm &);

	float *pos, *vel, *rtest;
	char *dumpdir;
	
};

//extern float *pos, *vel;

#endif

