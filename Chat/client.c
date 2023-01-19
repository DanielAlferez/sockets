#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_MESSAGE_LENGTH 256 // Tamaño máximo del mensaje a enviar o recibir

// Estructura del cliente
struct Client {
  int sockfd; // File descriptor del socket
  pthread_t send_thread;
  pthread_t recv_thread; 
  int key; // Llave para cifrar y descifrar mensajes utilizando César
};

// Función para cifrar mensajes utilizando César
void cesar_encrypt(char* text, int key) {
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        text[i] = (text[i] + key);
    }
}

// Función para descifrar mensajes utilizando César
void cesar_decrypt(char* text, int key) {
    int len = strlen(text);
    for (int i = 0; i < len; i++) {
        text[i] = (text[i] - key);
    }
}

// Función para manejar el hilo que envia mensajes
void* send_handler(void* arg) {
  struct Client* client = (struct Client*)arg;

    while (true) {
        char message[MAX_MESSAGE_LENGTH];
        // Recibe el mensaje del usuario a través del teclado
        fgets(message, MAX_MESSAGE_LENGTH, stdin);

        char messageFull[MAX_MESSAGE_LENGTH];
        char *inicio, *fin;
        // Busca una aparición de '(' y ')' en el mensaje
        inicio = strstr(message, "(");
        fin = strstr(message, ")");

        // Si el mensaje comienza con '!' y es 'u' o 'e'
        if (message[0] == '!' && (message[1] == 'u' || message[1] == 'e'))
        {
          if (message[1] == 'u')
          {
            // Envia el mensaje tal cual para listar los usuarios
            send(client->sockfd, message, strlen(message), 0);
          }else if (message[1] == 'e')
          {
            // Sale del programa
            exit(1);  
          }   
          // Si el mensaje tiene un '('  al inicio
        } else if (inicio != NULL && fin != NULL && message[0] == '(')
        {   
          //Se crean las variables para separar el nombre y el mensaje
          char name[32];

          //Copia la variable name
          strncpy(name, inicio + 1, fin - inicio - 1);
          name[fin-inicio-1] = '\0';

          //Se copia el mensaje sin el nombre
          strncpy(message, fin + 1 , MAX_MESSAGE_LENGTH);
          cesar_encrypt(message, client->key);
          sprintf(messageFull, "(%s)%s", name, message);

          // Envia el mensaje completo
          send(client->sockfd, messageFull, strlen(messageFull), 0);
        } // Si no es ni un comando ni un mensaje privado
        else{
          // Cifra el mensaje utilizando César
          cesar_encrypt(message, client->key);
          // Envia el mensaje cifrado
          send(client->sockfd, message, strlen(message), 0);
        }
      }
  return NULL;
}

// Función para manejar el hilo que recibe mensajes
void* recv_handler(void* arg) {
  struct Client* client = (struct Client*)arg;

    while (true) {
        char message[MAX_MESSAGE_LENGTH];
        int n;
        int bytes_received = recv(client->sockfd, message, MAX_MESSAGE_LENGTH, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        // Si el mensaje no comienza con '[' es un mensaje del servidor
        if (message[0] != '[')
        {
          // imprime el mensaje
          n = printf("%s", message);
          bzero(message, MAX_MESSAGE_LENGTH);
        }
        // Si el mensaje comienza con '[' es un mensaje privado
        else
        {
          char first[MAX_MESSAGE_LENGTH];
          char second[MAX_MESSAGE_LENGTH];
          char* p = strstr(message, ": ");
          // Se separa el nombre del usuario y el mensaje
          strncpy(first, message, p-message);
          first[p-message]='\0';
          strcpy(second,p+1);
          // Se decifra el mensaje
          cesar_decrypt(second, client->key);

          char final_string[MAX_MESSAGE_LENGTH];
          strcpy(final_string, first);
          strcat(final_string, ": ");
          strcat(final_string, second);
          // Si el mensaje es privado se imprime de un color diferente
          if (first[1] == '*')
          {
            sprintf(final_string, "\033[34m%s:\033[0m%s",first ,second);
          }else{
            sprintf(final_string, "\033[36m%s: \033[0m%s",first ,second);
          }
          n = printf("%s", final_string);
          bzero(final_string, MAX_MESSAGE_LENGTH);
        }       
        
        bzero(message, MAX_MESSAGE_LENGTH);
    }

  return NULL;
}

int main(int argc, char* argv[]) {
  
  if (argc != 3) {
    printf("Usage: %s <server_ip> <port>\n", argv[0]);
    return 1;
  }

  char* server_ip = argv[1];
  int port = atoi(argv[2]);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Error creating socket");
    return 1;
  }

  //Se crea la estructura del cliente
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server_ip);
  server_addr.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    return 1;
  }

  printf("\n\033[33mBienvenid@ al Chatroom!\033[0m\n");
  printf("\n\033[33m - Para enviar mensajes privados usar: '(nombre) mensaje' \033[0m\n");
  printf("\n\033[33m - Para mirar los usuarios conectados usar: '!u' \033[0m\n");
  printf("\n\033[33m - Para salir usar: '!e' \033[0m\n");
  srand(time(0));
  // Se genera un numero aleatorio para cuando haya un usuario con nombre repetido
  int numero_aleatorio = rand() % (100 - 1 + 1) + 1;
  char num_ale[20];
  sprintf(num_ale, "%d", numero_aleatorio);

  while (1)
  {
    char name[MAX_MESSAGE_LENGTH];
    char welcome[MAX_MESSAGE_LENGTH];
    //Pide al usuario ingresar el nombre
    printf("\n\033[33mIngresa tu nombre: \033[0m");
    fgets(name, MAX_MESSAGE_LENGTH, stdin);

    //Si el mensaje es "!e"
    if (name[0] == '!' &&  name[1] == 'e')
    {
      //Se envía el mensaje al servidor para cerrar la conexión
      send(sockfd, strtok(name, "\r\n"), MAX_MESSAGE_LENGTH, 0);
      //Se cierra el programa
      exit(0);
    }else{
      //Se envía el nombre
      send(sockfd, strtok(name, "\r\n"), MAX_MESSAGE_LENGTH, 0);
      recv(sockfd, welcome, MAX_MESSAGE_LENGTH, 0);  
    }

    //Si el servidor envía un "~" quiere decir que el nombre suministrado ya está en uso
    if (welcome[0] == '~')
    {
      //Se genera el nuevo nombre
      strcat(name, num_ale);
      printf("\n\033[31m%s\033[0m \033[33mSu nombre será: %s\033[0m\n", welcome, name);
      send(sockfd, strtok(name, "\r\n"), MAX_MESSAGE_LENGTH, 0);
      break;
    }

    //Si el servidor no envía un "#" quiere decir que el usuario se conectó correctamente
    else if (welcome[0] != '#')
    {
      printf("%s", welcome);
      break;
    }
    //Si el servidor envía un "#" quiere decir que el servidor está lleno
    else
    {
      printf("\n\033[31m%s\033[0m\n", welcome);
      bzero(welcome, MAX_MESSAGE_LENGTH);
    }
    bzero(name, MAX_MESSAGE_LENGTH);
  }
  
  struct Client client;
  client.sockfd = sockfd;
  client.key = 3;

  //Crea los hilos
  pthread_create(&client.recv_thread, NULL, recv_handler, &client);
  pthread_create(&client.send_thread, NULL, send_handler, &client);

  //Cierra los hilos
  pthread_join(client.send_thread, NULL);
  pthread_join(client.recv_thread, NULL);

  return 0;
}