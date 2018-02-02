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

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"


int main(int argc, const char* argv[]) {
    struct sockaddr_in server;
    int sock_id, len;
    char input[BUFFER];
    char output[BUFFER];
    // creating the socket
    if ((sock_id = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error in creating socket\n");
        exit(-1);
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    server.sin_addr.s_addr = inet_addr(argv[1]);
    bzero(&server.sin_zero, 8);

    // connecting TCP to server
    if (connect(sock_id, (struct sockaddr*) &server, sizeof(struct sockaddr_in) ) < 0) {
        perror("Error in connecting to server\n");
        exit(-1);
    }

    while (1) {
        // sending chat to client
        printf("%s", KGRN);
        printf("\nChat User#Client Input --> ");
        scanf("%[^\n]%*c", input);
        send(sock_id, input, BUFFER, 0);

        // check for exit condition
        if (strcasecmp(input, "bye") == 0) {
            break;
        }
        printf("%s", KRED);
        printf("\nChat User#Server Message :: ");
        fflush(stdout);

        // receing message from server
        len = recv(sock_id, output, BUFFER, 0);
        output[len] = '\0';
        if (strcasecmp(output, "bye") == 0) {
            break;
        }
        printf(" %s\n", output);
    }
    // close the socket
    printf("\nDisconnecting the chat service. !!!\n");
    close(sock_id);
}



