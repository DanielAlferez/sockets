#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 256
#define SERV_PORT 14000

void dg_cli(FILE *fp, int sockfd, struct sockaddr * pservaddr, long servlen)
{
    int n;
    char sendline[MAXLINE], recvline[MAXLINE + 1];
    while (fgets(sendline, MAXLINE, fp) != NULL)
    {
        sendto(sockfd, sendline, strlen(sendline), 0, pservaddr, servlen);
        n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
        recvline[n] = 0;
        fputs(recvline, stdout);
    }
}

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in serveraddr;
    if (argc != 2)
    {
        fprintf(stdout, "Uso: cliente <direccion IP>\n");
        exit(1);
    }
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = PF_INET;
    serveraddr . sin_port = htons(SERV_PORT);
    serveraddr .sin_addr .s_addr = inet_addr ( argv [1] ) ;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    dg_cli( stdin , sockfd , ( struct sockaddr*) &serveraddr , sizeof ( serveraddr ));

    exit (0) ;
}