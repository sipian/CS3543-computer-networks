#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#define BUFFER 1024

using namespace std;


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
	cout << "waiting for all clients to come" << endl;
	len = recv(sock_id, output, BUFFER, 0);
	if (len < 0 || !len) {
		printf("\nError in getting the id.\n Disconnecting the chat service. !!!\n");
		close(sock_id);
		return 0;
	}
	int id = atoi(output);
	cout << "starting the chat service" << endl;
	while (1) {

		if (id == 0) {
			printf("\nEnter Your Message :: ");
			scanf("%[^\n]%*c", input);
			len = send(sock_id, input, BUFFER, 0);
			if (!len || len < 0 || strcasecmp(input, "bye") == 0) {
				break;
			}
			printf("\n\t\tIncoming Message :: ");
			fflush(stdout);

			// receing message from server
			len = recv(sock_id, output, BUFFER, 0);
			if (!len || len < 0) {
				printf("bye\n");
				break;
			}
			output[len] = '\0';
			printf(" %s\n", output);
			if (strcasecmp(output, "bye") == 0) {
				break;
			}
		}
		else {
			printf("\n\t\tIncoming Message :: ");
			fflush(stdout);

			// receing message from server
			len = recv(sock_id, output, BUFFER, 0);
			if (!len || len < 0) {
				printf("bye\n");
				break;
			}
			output[len] = '\0';
			printf(" %s\n", output);
			if (strcasecmp(output, "bye") == 0) {
				break;
			}
			printf("\nEnter Your Message :: ");
			scanf("%[^\n]%*c", input);
			len = send(sock_id, input, BUFFER, 0);
			if (!len || len < 0 || strcasecmp(input, "bye") == 0) {
				break;
			}
		}

		// check for exit condition
	}
	// close the socket
	printf("\nDisconnecting the chat service. !!!\n");
	close(sock_id);
}



