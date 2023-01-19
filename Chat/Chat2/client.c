#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_MESSAGE_LENGTH 256

struct Client {
  int sockfd;
  pthread_t send_thread;
  pthread_t recv_thread;
};

void* send_handler(void* arg) {
  struct Client* client = (struct Client*)arg;

  // Continuously prompt user for a message to send to the server
  while (true) {
    char message[MAX_MESSAGE_LENGTH];
    printf("Enter a message to send: ");
    fgets(message, MAX_MESSAGE_LENGTH, stdin);
    send(client->sockfd, message, strlen(message), 0);
  }

  return NULL;
}

void* recv_handler(void* arg) {
  struct Client* client = (struct Client*)arg;

  // Continuously receive messages from the server and print them to the screen
  while (true) {
    char message[MAX_MESSAGE_LENGTH];
    int bytes_received = recv(client->sockfd, message, MAX_MESSAGE_LENGTH, 0);
    if (bytes_received <= 0) {
      break;
    }
    printf("Received message: %s", message);
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

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(server_ip);
  server_addr.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    return 1;
  }

  printf("Connected to server!\n");

  struct Client client;
  client.sockfd = sockfd;
  pthread_create(&client.send_thread, NULL, send_handler, &client);
  pthread_create(&client.recv_thread, NULL, recv_handler, &client);

  pthread_join(client.send_thread, NULL);
  pthread_join(client.recv_thread, NULL);

  return 0;
}
