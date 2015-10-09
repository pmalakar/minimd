#ifndef DUMP_H_
#define DUMP_H_

#include "atom.h"
#include "comm.h"

//extern FILE *dumpfp;

void initDump(Comm &);
void dump(Atom &, int, Comm &);
void finiDump();

#endif

