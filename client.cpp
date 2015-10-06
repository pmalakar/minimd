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

#include "client.h"

char *ipaddress;

int sockfd = 0, n = 0;
char recvBuff[1024];
char sendBuff[1024];
time_t ticks; 
struct sockaddr_in serv_addr; 

//int main(int argc, char *argv[]) {}
int initConnection()
{

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

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
		else {
       printf("\n Connection Successful \n");
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
    snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
    n = write(sockfd, sendBuff, strlen(sendBuff)); 
    if(n < 0)
    {
        printf("\n Write error \n");
    } 

    return 0;

}

int sendData() {

		double data[10];
		for (int i=0; i<10 ; i++) {
    	snprintf(sendBuff, sizeof(sendBuff), "%5.2lf\n", data[i]);
    	n = write(sockfd, sendBuff, strlen(sendBuff)); 
		}

    if(n < 0)
    {
        printf("\n Write error \n");
    } 

    return 0;

}


