#ifndef CLIENT_H_
#define CLIENT_H_

extern char *ipaddress;

int initConnection(); //int, char*);
void writeRemote(Atom &, int, Comm &);
void finiConnection(); //int, char*);

#endif

