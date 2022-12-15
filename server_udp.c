#define MAXLINE 256
#define SERV_PORT 14000
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void dg_echo(int sockfd , struct sockaddr *pcliaddr , long clilen )
{
    int n;
    socklen_t len ;
    char msg[MAXLINE];
    for (;;) {
        len = clilen ;
        n = recvfrom(sockfd , msg, MAXLINE, 0, pcliaddr, &len);
        sendto(sockfd , msg, n , 0, pcliaddr , len);
    }
}

int main( int argc , char **argv)
{
    int sockfd;
    struct sockaddr_in serveraddr , cliaddr ;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr . sin_family = PF_INET;
    serveraddr .sin_addr .s_addr = htonl(INADDR_ANY);
    serveraddr . sin_port = htons(SERV_PORT);
    bind(sockfd , ( struct sockaddr*) &serveraddr , sizeof ( serveraddr ));
    dg_echo(sockfd , ( struct sockaddr*) &cliaddr , sizeof ( cliaddr ));
}