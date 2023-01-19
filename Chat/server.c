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

#define MAX_USERS 10 //Define el número máximo de usuarios que pueden conectarse al mismo tiempo.
#define MAX_MESSAGE_LENGTH 256 //Define la longitud máxima permitida para los mensajes enviados.

// Crea la estructura para el usuario
typedef struct 
{
    char name[32];
    int socket;
} User;

// Variables globales
User users[MAX_USERS];
int num_users = 0;

pthread_mutex_t users_mutex;

// Funcion que busca si ya existe el nombre ingresado
bool nameExist(char *username){
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        //Se comparan los nombres
        if(strcmp(user.name, username) == 0){
           return true;
        }
    }
    return false;
}

// Función para enviar un mensaje a todos los usuarios conectados
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

// Función que envía un mensaje a todos los usuarios cuando alguien se ha conectado
void user_connected(char *username){
    char message[MAX_MESSAGE_LENGTH];
    pthread_mutex_lock(&users_mutex);
    sprintf(message, "\n\033[32m*** %s se ha unido ***\033[0m\n\n", username);
    printf("\n%s se ha unido\n", username);
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

// Función que envía un mensaje a todos los usuarios cuando alguien se ha desconectado
void user_disconnected(char *username){
    char message[MAX_MESSAGE_LENGTH];
    pthread_mutex_lock(&users_mutex);
    sprintf(message, "\n\033[31m*** %s se ha salido ***\033[0m\n\n", username);
    printf("\n%s se ha salido\n", username);
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

// Función para enviar mensajes privados entre usuarios
void private_message(char *receiver, char *message, char *sender){
    char messageFull[MAX_MESSAGE_LENGTH];
    pthread_mutex_lock(&users_mutex);
    bool notFound = 1;
    User userSender;
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        // Se verifica que el receptor del mensaje privado exista
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

// Función para listar los usuarios
void showUsers(char *sender, char *message){
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < num_users; i++)
    {
        User user = users[i];
        //Se envia el mensaje menos al que lo envió
        if(strcmp(user.name, sender) == 0){
            send(user.socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&users_mutex);
}

// Función para manejar el hilo del servidor
void *handle_client(void *arg){
    int socket = *(int *)arg;
    char message[MAX_MESSAGE_LENGTH];
    int received;

    while (1)
    {      
        bzero(message, MAX_MESSAGE_LENGTH);
        received = recv(socket, message, MAX_MESSAGE_LENGTH, 0);

        //Permite ingresar solo si el servidor no está lleno
        if (num_users == MAX_USERS)
        {
            char full[MAX_MESSAGE_LENGTH];
            strcat(full, "# Servidor lleno #");
            send(socket, full, strlen(full), 0);
            bzero(full, MAX_MESSAGE_LENGTH);
            if (message[0] == '!' && message[1] == 'e')
            {
                close(socket);
                return NULL;
            }
        }
        // Si el nombre ya existe
        else if (nameExist(message))
        {
            char full2[MAX_MESSAGE_LENGTH];
            strcat(full2, "~ El nombre está en uso ~");
            send(socket, full2, strlen(full2), 0);
            bzero(full2, MAX_MESSAGE_LENGTH);

        }
        // Si el mensaje fue recibido exitosamente
        else if (received > 0) 
        {
            message[received] = '\0';
            break;
        }else       
        {
            close(socket);
            return NULL;
        }
    }
    
    char *username = strdup(message);
    //Limpiar username
    for (int i = 0; i < strlen(username); i++)
    {
        if (username[i] == '\r')
        {   
            username[i] = '\0';
            break;
        }
    }

    // Se agrega el usuario al arreglo
    pthread_mutex_lock(&users_mutex);
    // Se crea la estructura
    User user;
    strcpy(user.name, username);
    user.socket = socket;
    //Se guarda la estructura creada en la ultima posición del arreglo
    users[num_users++] = user;
    pthread_mutex_unlock(&users_mutex);

    sprintf(message, "\n\033[32m%s bienvenid@ al chat\033[0m\n\n", username);
    send(socket, message, strlen(message), 0);
    user_connected(strtok(username, "\r\n"));

    //While para recivir los mensajes y transmitirlos a los demas clientes
    while (1)
    {
        char message[MAX_MESSAGE_LENGTH];
        char *inicio, *fin;

        received = recv(socket, message, MAX_MESSAGE_LENGTH, 0);
        if (received <= 0)
        {
            perror("No se pudo seguir enviando mensajes");
            break;
        }
        message[received] = '\0';

        char messageFull[MAX_MESSAGE_LENGTH];
        char names[MAX_MESSAGE_LENGTH];
        // Busca "(" y ")" en el mensaje, lo que equivale a un mensaje privado
        inicio = strstr(message, "(");
        fin = strstr(message, ")");

        // Si el mensaje empieza con "!" se refiere a listar los usuarios
        if (message[0] == '!')
        {
            // Se crea la cadena de texto con los usuarios
            for (int i = 0; i < num_users; i++)
            {
                User user = users[i];
                strcat(names, "[");
                strcat(names, user.name);
                strcat(names, "] ");
            }
            sprintf(messageFull, "\033[35m%s\033[0m\n", names);
            showUsers(username, messageFull);
            bzero(names, MAX_MESSAGE_LENGTH);
        } else if (inicio != NULL && fin != NULL && message[0] == '(')
        {
            //Se crean las variables para separar el nombre y el mensaje
            char name[32];

            //Copia la variable name
            strncpy(name, inicio + 1, fin - inicio - 1);
            name[fin-inicio-1] = '\0';

            //Se copia el mensaje sin el nombre
            strncpy(message, fin + 1 , MAX_MESSAGE_LENGTH);
            
            // Si el mensaje es privado se agregan "*" al inicio y final del nombre
            sprintf(messageFull, "[*%s*]: %s", username, message);
            private_message(name, messageFull, username);
        }else{
            sprintf(messageFull, "[%s]: %s", username, message);
            broadcast_message(username, messageFull);
        } 
        bzero(messageFull, MAX_MESSAGE_LENGTH);
    }

    // Se elimina el usuario del arreglo
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
    user_disconnected(username);
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
