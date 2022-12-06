#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
  char* resource;
  char* type;
}Data;

int ValidateRequest(char request[], Data* data);
int Send(char* path, char *file_name, char *buffer, char *type, int *lenth);
int SendPost(char *buffer);
 
int main(int argc, char** argv) {
 
  char *ip = "127.0.0.1";
  int port = 6605;
 
  int server_sock, client_sock;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_size;
  char buffer[2048];
  char type[15];
  int n;
 
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == server_sock) {
    perror("[-]Socket creation failed.\n");
    exit(1);
  }
  printf("[+]TCP server socket created.\n");
 
  memset(&server_addr, '\0', sizeof(server_addr));
  memset(&client_addr, '\0', sizeof(client_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);
 
  n = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
  if (0 != n) {
    perror("[-]Bind failed\n");
    exit(1);
  }
  printf("[+]Bind to the port number: %d\n", port);
 
  listen(server_sock, 5);
  printf("Listening...\n");
 
  while(1) {
    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    printf("[+]Client connected.\n");
 
    bzero(buffer, 2048);
    read(client_sock, buffer, sizeof(buffer));
    printf("From client: %s\n", buffer);
    
    Data data;
    int res = ValidateRequest(buffer, &data);
    int length;
    
    if (-1 == res) {
    	sprintf(buffer, "HTTP/1.1 501 Not Implemented\n");
    } else {
      if (strstr(data.resource, ".txt")) {
        strcpy(type, "text/plain");
      } else if (strstr(data.resource, ".html")) {
        strcpy(type, "text/html");
      } else if (strstr(data.resource, ".jpeg") || strstr(data.resource, ".jpg")) {
        strcpy(type, "image/jpeg");
      } else {
        printf("Not a valid type file.\n");
      }
    
      if (strcmp(data.type, "GET") == 0) { 
      	Send(argv[1], data.resource, buffer, type, &length);
      } else if (strcmp(data.type, "POST") == 0) { 
      	SendPost(buffer);
      }
    }

    write(client_sock, buffer, length);
 
    close(client_sock);
    printf("[+]Client disconnected.\n\n");
 
  }
  return 0;
}

int SendPost(char *buffer) {
  sprintf(buffer, "HTTP/1.1 200 OK\n");
  
  return 0;
}

int Send(char* path, char *file_name, char *buffer, char *type, int *length) {
  char *file_path;
  file_path = (char *)malloc(100 * sizeof(char));
  file_path[0] = '\0';
  strncat(file_path, path, strlen(path));
  strncat(file_path, file_name, strlen(file_name));
  
  FILE* ptr = fopen(file_path, "r");
  free(file_path);
  
  char *content;
  int i = 0;
  
  if (NULL == ptr) {
    printf("file can't be opened \n");
    bzero(buffer, 25);
    sprintf(buffer, "HTTP/1.1 404 Not Found\n");
    return -1;
  }
  
  fseek(ptr, 0, SEEK_END);
  int len = ftell(ptr);
  fseek(ptr, 0, SEEK_SET);
  
  content = (char *)malloc(len * sizeof(char));
  
  do {
      content[i] = fgetc(ptr);
      i++;
  } while (i < len);

  fclose(ptr);
  
  bzero(buffer, (len + 100));
  
  sprintf(buffer, "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %d\n\n\0", type, len);
  int buff_len = strlen(buffer);
  *length = (len + buff_len);
  
  for(int j = 0; j < len; j++) {
  	buffer[j + buff_len] = content[j];
  }
  free(content);
  
  return 0;
}

int ValidateRequest(char request[], Data* data) {
  char *request_parts = strtok(request, " ");
  data->type = request_parts; 
  
  if (strcmp(request_parts, "GET") == 0 || strcmp(request_parts, "POST") == 0) {
    request_parts = strtok(NULL, " /"); 
    data->resource = request_parts;
    request_parts = strtok(NULL, "\r\n"); 
    if (strcmp(request_parts, "HTTP/1.1") == 0) {
      request_parts = strtok(NULL, " "); 
      if (strcmp(request_parts, "\nHost:") == 0) {
        request_parts = strtok(NULL, " "); 
        return 0;
      }
    }
  }
  
  return -1;
}

