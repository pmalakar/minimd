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

#include "ipaddr.h"

int main(int argc, char *argv[])
{
    int i, n, listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr; 

    char sendBuff[1024];
    char recvBuff[1024];

		float position[20000];
    time_t ticks; 
		int size[1];

		getIP();
		fflush(stdout);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 
    memset(recvBuff, '0', sizeof(recvBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(listenfd, 10); 

    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
		n = read(connfd, recvBuff, sizeof(recvBuff)-1);
		while (1)	//(n = read(connfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
/*
					n = read(connfd, recvBuff, sizeof(recvBuff)-1);
        	recvBuff[n] = '\0';
					if (strcmp (recvBuff, "End") == 0) break;
        	if(fputs(recvBuff, stdout) == EOF)
           	printf("\n Error : Fputs error\n");
*/
				//	n = read(connfd, size, sizeof(size)-1);
					//n = read(connfd, position, sizeof(position)-1);
					n = read(connfd, position, 10);
					if (n>0) {
						printf("read %d bytes: %d\n", n, sizeof(position));
						for (i=0; i<1; i++) printf("Read pos[%d] %f\n", i, position[i]);	
					}
					fflush(stdout);
    } 

    close(connfd);
		printf("Connection closed\n\n");
    sleep(1);

}


