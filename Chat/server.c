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
#include <stdbool.h>

#define MAX_USERS 3
#define MAX_MESSAGE_LENGHT 256

typedef struct 
{
    char name[32];
    int socket;
} User;

User users[MAX_USERS];
int num_users = 0;

pthread_mutex_t users_mutex;

void broadcast_message(char *sender, char *message){

    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        //Se envia el mensaje menos al que lo envió
        if(strcmp(user.name, sender) != 0){
            send(user.socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void user_connected(char *username){
    char message[MAX_MESSAGE_LENGHT];
    pthread_mutex_lock(&users_mutex);
    sprintf(message, "\n%s se ha unido\n", username);
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        //Se envia el mensaje menos al que lo envió
        if(strcmp(user.name, username) != 0){
            send(user.socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

void private_message(char *receiver, char *message, char *sender){
    char messageFull[MAX_MESSAGE_LENGHT];
    pthread_mutex_lock(&users_mutex);
    bool notFound = 1;
    User userSender;
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        //Se envia el mensaje solo al receptor
        if(strcmp(user.name, receiver) == 0){
            send(user.socket, message, strlen(message), 0);
            notFound = 0;
        }
        
        if (strcmp(user.name, sender) == 0)
        {
            userSender = users[i];
        }
    }  

    if (notFound)
    {
        char* notFoundMessage = "El usuario destinatario no se encuentra en el chat\n";
        send(userSender.socket, notFoundMessage, strlen(notFoundMessage), 0);  
    }

    
    pthread_mutex_unlock(&users_mutex);
}

void *handle_client(void *arg){
    int socket = *(int *)arg;
    char message[MAX_MESSAGE_LENGHT];
    int received;

    //Permite ingresar solo si el servidor no está lleno
    while (1)
    {      
        received = recv(socket, message, MAX_MESSAGE_LENGHT, 0);
        if (num_users == MAX_USERS)
        {
            char *full = "Servidor lleno";
            send(socket, full, strlen(full), 0);
            continue;
        }else if (received < 0)
        {
            perror("Error al recibir el nombre");
            return NULL;
        }else{
            message[received] = '\0';
            break;
        }
    }
    

    char *username = strdup(message);
    //Limpiar username
    for (int i = 0; i < strlen(username); i++)
    {
        printf("%c a", username[i]);
        if (username[i] == '\r')
        {   
            username[i] = '\0';
            break;
        }
    }
    
    printf("%s\n", username);

    pthread_mutex_lock(&users_mutex);
    User user;
    strcpy(user.name, username);
    user.socket = socket;
    users[num_users++] = user;
    pthread_mutex_unlock(&users_mutex);

    sprintf(message, "%s Bienvenido al chat\n", username);
    send(socket, message, strlen(message), 0);
    user_connected(username);


    while (1)
    {
        char message[MAX_MESSAGE_LENGHT];
        char *inicio, *fin;

        // send(socket, "[Tú]: ", strlen("\n[Tú]: "), 0);
        received = recv(socket, message, MAX_MESSAGE_LENGHT, 0);
        if (received <= 0)
        {
            perror("No se pudo seguir enviando mensajes");
            break;
        }
        message[received] = '\0';

        char messageFull[MAX_MESSAGE_LENGHT];

        inicio = strstr(message, "(");
        fin = strstr(message, ")");

        if (inicio != NULL && fin != NULL && message[0] == '(')
        {   
            //Se crean las variables para separar el nombre y el mensaje
            char name[32];

            //Copia la variable name
            strncpy(name, inicio + 1, fin - inicio - 1);
            name[fin-inicio-1] = '\0';

            //Se copia el mensaje sin el nombre
            strncpy(message, fin + 2 , MAX_MESSAGE_LENGHT);

            sprintf(messageFull, "[%s]: %s", username, message);
            private_message(name, messageFull, username);
        }else{
            sprintf(messageFull, "[%s]: %s", username, message);
            broadcast_message(username, messageFull);
        }
        
    }


    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < num_users; i++)
    {
        if(strcmp(users[i].name, username) == 0){
            for (int j = i; j < num_users -1; j++)
            {
                users[j]=users[j+1];
            }
            num_users--;
            break;
        }
    }
    pthread_mutex_unlock(&users_mutex);

    close(socket);
    free(username);
    return NULL;
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

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof (server_address)) < 0 ) {
        perror("Error al asociar el socket del servidor a la direccion");
        return 1;
    }

    if (listen(server_socket, MAX_USERS) < 0)
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