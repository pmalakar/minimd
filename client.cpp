#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <time.h> 

#include "comm.h"
#include "atom.h"
#include "client.h"
#include "dump.h"

char *ipaddress = NULL;

int sockfd = 0, numbytes = 0;
char recvBuff[1024];
char sendBuff[1024];
time_t ticks; 
struct sockaddr_in serv_addr; 

int flag = 0;

//int main(int argc, char *argv[]) {}
int initConnection()
{

		flag = 0;

    if(ipaddress == NULL)
    {
        printf("\n Usage: ... -ip <ip of server>\n");
        return 1;
    } 

    memset(sendBuff, '0', sizeof(sendBuff));
    memset(recvBuff, '0', sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 

    if(inet_pton(AF_INET, ipaddress, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
       printf("\nError : Connect Failed \n");
       return 1;
    } 
		else {
       printf("\nConnection Successful\nSize of sendBuff=%d\n", sizeof(sendBuff));
		}

/*
    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 
*/

		ticks = time(NULL);
    snprintf(sendBuff, sizeof(sendBuff), "%.24s\n", ctime(&ticks));
    numbytes = write(sockfd, sendBuff, strlen(sendBuff)); 
    if(numbytes < 0)
    {
        printf("\nInitial write error \n");
    } 

    return 0;

}

int send(Atom &atom, int n, Comm &comm, Dump &dump) {

		printf("Inside send\n"); //, size[0], pos[0], pos[1], pos[2]);
		if (flag == 1) return 1;

		printf("Inside send\n"); //, size[0], pos[0], pos[1], pos[2]);
		if (sockfd == NULL) {
			printf("\nsocket fd null\n");
			return 1;
		}

/*
		double data[10];
		for (int i=0; i<1 ; i++) {
			if (n == 20) { 
	    	snprintf(sendBuff, sizeof(sendBuff), "End");
				flag = 1;
			}
			else
    		snprintf(sendBuff, sizeof(sendBuff), "%d:%d: %5.2lf\n", comm.me, n, data[i]);

    	numbytes = write(sockfd, sendBuff, strlen(sendBuff)); 
    	if(numbytes < 0)
         printf("\nWrite error %d %s\n", errno, strerror(errno));
			else
         printf("\nWrite success\n");
		}
*/

		int size[1];
		size[0] = 3*atom.nlocal;

		printf("Wrote %d %f %f %f\n", size[0], dump.pos[0], dump.pos[1], dump.pos[2]);
		fflush(stdout);
		if (n < 2) {
    //	numbytes = write(sockfd, size, 1);
    	//numbytes = write(sockfd, pos, size[0]*sizeof(float)); //3*atom.nlocal);
    	numbytes = write(sockfd, dump.pos, sizeof(float)) ; //size[0]*sizeof(float)); //3*atom.nlocal);
    	if(numbytes < 0)
         printf("\nWrite error %d %s\n", errno, strerror(errno));
			else
         printf("\nWrite success %d\n", numbytes);
			
		}

    return 0;

}

void writeRemote(Atom &atom, int n, Comm &comm, Dump &dump) {
	
	dump.pack(atom, n, comm);
	if (comm.me == 0) 
	send(atom, n, comm, dump);
	dump.unpack(); //atom, n, comm);
}

void finiConnection() {
	close(sockfd);
}

