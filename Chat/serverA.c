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
#include <pthread.h>

#define MAX_USERS 10
#define MAX_MESSAGE_LENGHT 256

void *handle_client(void *arg){
    int socket = *(int *)arg;
    char message[MAX_MESSAGE_LENGHT];

    strcpy(message, "\nIngresa tu nombre: ");
    send(socket, message, strlen(message), 0);

    int received = recv(socket, message, MAX_MESSAGE_LENGHT, 0);
    if (received < 0)
    {
        perror("Error al recibir el nombre");
    }
    
}


int main(int argc, char const *argv[])
{
    if (argc < 2){
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
    }

    int port = atoi(argv[1]);

    int server_socket = socket(AF_INET, SOCK_STREAM,0);
    if (server_socket < 0)
    {
        perror("Error al crear el socket del servidor");
        return 1;
    }
    
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if ( bind ( server_socket, ( struct sockaddr*) &server_address , sizeof (server_address)) < 0 ) {
        perror("Error al asociar el socket del servidor a la direccion");
        return 1;
    }

    if (listen(server_socket, 10) < 0)
    {
        perror("Error al escuchar");
        return 1;
    }

    printf("Servidor iniciado en el puerto %d\n", port);

    while (1)
    {
        //Conexión Cliente
        struct sockaddr_in client_address;
        socklen_t clilen = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &clilen);
        if (client_socket < 0)
        {
            perror("Error al aceptar la conexion");
            continue;
        }
        
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, &client_socket) != 0){
            perror("Error al crear el hilo del cliente");
            continue;
        }

    }
    
}
