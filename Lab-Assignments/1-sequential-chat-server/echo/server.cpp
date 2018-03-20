#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 10000
#define ERROR -1
#define MAX_CLIENTS 2
#define MAX_DATA 1024

int main(int argc, const char* argv[]) {
    int sock_id, client_id;
    struct sockaddr_in server , client;
    socklen_t len = sizeof(struct sockaddr_in), data_len;

    char data[MAX_DATA];
    // creating the socket
    if((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in making the socket.\n");
        exit(-1);
    }
   

    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);

    // binding TCP server to IP and port
    if( bind(sock_id, (struct sockaddr*) &server, len) < 0) {
        perror("Bind failed\n");
        exit(-1);
    }

    // mark it for listening
    if( listen(sock_id, MAX_CLIENTS)) {
        perror("Error in listening\n");
        exit(-1);
    }
    printf("Started Listening\n");
    while(1) {
        
        if((client_id = accept(sock_id, (struct sockaddr*) &client, &len)) < 0) {
            printf("Error in accepting\n");
            exit(-1);
        }
        data_len = 1;
        while(data_len) {

            // receing message from client
            data_len = recv(client_id, data, MAX_DATA, 0);
            if(data_len) {

                // sending chat to client
                send(client_id, data, data_len, 0);
                data[data_len]  = '\0';
                printf("\nClient has sent the following messsage : %s", data);
                fflush(stdout);
            }
        }
        printf("\nClient disconnected\n");
        close(client_id);
    }
    close(sock_id);

    return 0;
}
