#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define PORT 10000
#define ERROR -1
#define MAX_CLIENTS 2
#define BUFFER 1024

int main(int argc, const char* argv[]) {
    int sock_id, client_id;
    struct sockaddr_in server , client;
    socklen_t len = sizeof(struct sockaddr_in), data_len;

    // creating the socket
    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in making the socket.\n");
        exit(-1);
    }


    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 0);
    
    // binding TCP server to IP and port
    if ( bind(sock_id, (struct sockaddr*) &server, len) < 0) {
        perror("Bind failed\n");
        exit(-1);
    }

    // mark it for listening
    if ( listen(sock_id, MAX_CLIENTS)) {
        perror("Error in listening\n");
        exit(-1);
    }
    char input[BUFFER];
    char data[BUFFER];

    if ((client_id = accept(sock_id, (struct sockaddr*) &client, &len)) < 0) {
        printf("Error in accepting\n");
        exit(-1);
    }
    data_len = 1;
    while (data_len) {
        printf("%s", KRED);
        printf("\nChat User#Client Message :: ");
        fflush(stdout);

        // receing message from client
        data_len = recv(client_id, data, BUFFER, 0);
        if (strcasecmp(data, "bye") == 0) {
            break;
        }
        if (data_len) {
            printf("%s\n", data);
            printf("%s", KGRN);
            printf("\nChat User#Server Input --> ");
            scanf("%[^\n]%*c", input);

            // sending chat to client
            send(client_id, input, data_len, 0);

            if (strcasecmp(input, "bye") == 0) {
                break;
            }

        }
    }
    // close the connection
    printf("\nDisconnecting the chat service for this client listening for another client. !!!\n");
    close(client_id);

    // close the socket
    close(sock_id);

    return 0;
}
