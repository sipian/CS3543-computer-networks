#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER 1024

int main(int argc, const char* argv[]) {
    struct sockaddr_in server;
    int sock_id, len;
    char input[BUFFER];
    char output[BUFFER];
    // creating the socket
    if((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in creating socket\n");
        exit(-1);
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);
    bzero(&server.sin_zero, 8);

    // connecting TCP to server
    if(connect(sock_id, (struct sockaddr*) &server, sizeof(struct sockaddr_in) ) < 0) {
        perror("Error in connecting to server\n");
        exit(-1);
    }
    char exit[] = "exit";
    while(1) {
        printf("\nEnter the input to be sent to server: ");
        scanf("%[^\n]%*c", input);
        if(strcasecmp(input, exit) == 0) {
            break;
        }
        // sending chat to server
        send(sock_id, input, BUFFER, 0);
        len = recv(sock_id, output, BUFFER, 0);
        output[len] = '\0';
        printf("\tReceived the following message from the server : %s", output);
    }
    close(sock_id);
}



