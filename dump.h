#ifndef DUMP_H_
#define DUMP_H_

#include "atom.h"
#include "comm.h"

//extern FILE *dumpfp;

void initDump(Comm &);

void pack(Atom &, int, Comm &);
void unpack(void);

void writeFile(Atom &, int, Comm &);
void finiDump(Comm &);

extern float *pos, *vel;

#endif

