#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAXLINE 256
int sock;
struct sockaddr_in addr;


void *sendMessage(){
    char sendline[MAXLINE];
    while (1)
    {
        fgets(sendline, MAXLINE, stdin);    
        char *message = strtok(sendline, "\r\n");
        send(sock, message, strlen(message), 0);
        bzero(message, MAXLINE);
    }
}

void *recvMessage(){
    char recvline[MAXLINE];
    while (1)
    {
        int receive = recv(sock, recvline, MAXLINE, 0);
        printf("%s", recvline);     
        memset(recvline, 0, sizeof(recvline));
    }
}

int main( int argc , char *argv [] )
{     
    struct hostent *hp, *gethostbyname();

    if (argc!=3){
        fprintf(stderr, "Uso:%s<host> <port>\n", argv[0]);
        exit(1);
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Imposible creacion del socket");
        exit(2);
    }

    if((hp = gethostbyname(argv[1])) == NULL){
        perror("Error: Nombre de la maquina desconocido");
        exit(3);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[2]));
    // bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        perror("Not Connect");
        exit(4);
    }

    char name[MAXLINE];
    printf("\nIngresa tu nombre: ");
    fgets(name, MAXLINE, stdin);
    // char nameCleaned = strtok(name, "\r\n");
    send(sock, name, MAXLINE, 0);

    pthread_t sendM, recvM;

    pthread_create(&sendM, NULL, sendMessage, NULL);
    pthread_create(&recvM, NULL, recvMessage, NULL);
}