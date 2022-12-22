#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define SIZE 256
#define MAXLINE 4096

void str_echo(FILE*fp, int sock );

int main( int argc , char *argv [] )
{     
    int sock;
    char com[SIZE];
    struct sockaddr_in adr;
    struct hostent *hp, *gethostbyname();

    if (argc!=3){
        fprintf(stderr, "Uso:%s<host><port>\n", argv[0]);
        exit(1);
    }

    if((sock = socket (PF_INET, SOCK_STREAM, 0 ))== -1){
        perror( "Imposible creacion del socket");
        exit(2);
    }

    if((hp = gethostbyname(argv [1])) == NULL){
        perror("Error: Nombre de la maquina desconocido");
        exit(3);
    }

    adr.sin_family = PF_INET;
    adr.sin_addr.s_addr = htonl(INADDR_ANY);
    adr.sin_port = htons (atoi(argv[2]));
    bcopy(hp->h_addr, &adr.sin_addr, hp->h_length);

    if(connect(sock, &adr, sizeof(adr)) == -1 ) {
        perror("connect");
        exit(4);
    }

    str_echo(stdin, sock);
}


void str_echo(FILE*fp, int sock)
{
    char sendline[MAXLINE], recvline[MAXLINE];
    int n;
    while(fgets(sendline, MAXLINE, fp )!= NULL ){
        write(sock, sendline, strlen(sendline) ) ;
        if(read(sock, recvline, MAXLINE )== 0) {
            fprintf(stderr , "Servidor terminado prematuramente\n" );
            exit(5);
        }
        fputs(recvline, stdout);
    }
}
