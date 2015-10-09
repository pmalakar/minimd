#ifndef CLIENT_H_
#define CLIENT_H_

extern char *ipaddress;
extern int sockfd;

int initConnection(); //int, char*);
int sendData(void);
void finiConnection(); //int, char*);

#endif

