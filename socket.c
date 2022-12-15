#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

// #define SERV_HOST_ADDR "10.0.2.15"

int main(int argc, char const *argv[])
{
    int skt, skt2;
    int longitud_cliente, puerto = 9999;
    struct sockaddr_in server, cliente;
    
    bzero(&(server.sin_zero),8);
    server.sin_family = AF_INET;
    server.sin_port = htons(puerto);
    server.sin_addr.s_addr = htonl(INADDR_ANY); //Se puede conectar cualquiera

    skt = socket(AF_INET, SOCK_STREAM,0);
    printf("Creando el Socket");

    bind(skt, (struct sockaddr*)&server, sizeof(struct sockaddr)); //Se realiza la asociacion - Se asocia el socket al server
    printf("Bind realizado");

    listen(skt, 5);
    printf("Esperando conexiones");

    
    while (1)
    {
        longitud_cliente = sizeof(struct sockaddr_in);
        printf("Esperando");
        skt2 = accept(skt, (struct sockaddr*)&cliente, &longitud_cliente);
    }
    
    int close(int skt);

    return 0;
}
