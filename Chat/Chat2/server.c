#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_MESSAGE_LENGTH 256
#define MAX_CLIENTS 10

struct Message {
  char text[MAX_MESSAGE_LENGTH];
  int sender_id;
};

struct Client {
  int id;
  char name[MAX_MESSAGE_LENGTH];
  int sockfd;
  pthread_t thread;
  struct Server* server;
};

struct Server {
  int listen_sockfd;
  int client_count;
  struct Client* clients;
  pthread_mutex_t message_queue_mutex;
  pthread_cond_t message_available;
  struct Message message_queue[MAX_CLIENTS];
  int message_count;
};

void broadcast(struct Server* server, char* message, int sender_id) {
  pthread_mutex_lock(&server->message_queue_mutex);
  for (int i = 0; i < server->client_count; i++) {
    if (server->clients[i].id != sender_id) {
      send(server->clients[i].sockfd, message, strlen(message), 0);
    }
  }
  pthread_mutex_unlock(&server->message_queue_mutex);
}

void* client_handler(void* arg) {
  struct Client* client = (struct Client*)arg;
  struct Server* server = client->server;

  // Request client's name
  send(client->sockfd, "Enter your name: ", 18, 0);
  recv(client->sockfd, client->name, MAX_MESSAGE_LENGTH, 0);

  // Continuously receive messages from client and broadcast them to all other clients
  while (true) {
    char message[MAX_MESSAGE_LENGTH];
    int bytes_received = recv(client->sockfd, message, MAX_MESSAGE_LENGTH, 0);
    if (bytes_received <= 0) {
      break;
    }

    char broadcast_message[MAX_MESSAGE_LENGTH];
    sprintf(broadcast_message, "[%s]: %s", client->name, message);
    broadcast(server, broadcast_message, client->id);
  }

  // Remove client from server
  close(client->sockfd);
  pthread_mutex_lock(&server->message_queue_mutex);
  for (int i = 0; i < server->client_count; i++) {
    if (server->clients[i].id == client->id) {
      server->clients[i] = server->clients[server->client_count - 1];
      server->client_count--;
      break;
    }
  }
  pthread_mutex_unlock(&server->message_queue_mutex);

  return NULL;
}


int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s <port>\n", argv[0]);
    return 1;
  }

  int port = atoi(argv[1]);

  struct Server server;
  server.listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (server.listen_sockfd < 0) {
    perror("Error creating socket");
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server.listen_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    return 1;
  }

  if (listen(server.listen_sockfd, MAX_CLIENTS) < 0) {
    perror("Error listening on socket");
    return 1;
  }

  server.client_count = 0;
  server.clients = malloc(MAX_CLIENTS * sizeof(struct Client));
  pthread_mutex_init(&server.message_queue_mutex, NULL);
  pthread_cond_init(&server.message_available, NULL);

  // Continuously accept new connections and create a new thread for each client
  while (true) {
       struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_sockfd = accept(server.listen_sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_sockfd < 0) {
      perror("Error accepting connection");
      continue;
    }

    pthread_mutex_lock(&server.message_queue_mutex);
    int id = server.client_count++;
    struct Client* client = &server.clients[id];
    client->id = id;
    client->sockfd = client_sockfd;
    client->server = &server;
    pthread_create(&client->thread, NULL, client_handler, client);
    pthread_mutex_unlock(&server.message_queue_mutex);
  }

  return 0;
}
