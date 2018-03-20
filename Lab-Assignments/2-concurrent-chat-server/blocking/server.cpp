#include <stdio.h>
#include <string.h>
#include <iostream>
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
#define BUFFER 1024
using namespace std;

int main(int argc, const char* argv[]) {
	int sock_id;
	struct sockaddr_in server;
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
	cout << "started listening on port " << argv[1] << endl;
	char input[BUFFER];
	char data[BUFFER];

	int client_id[MAX_CLIENTS];
	struct sockaddr_in client[MAX_CLIENTS];

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if ((client_id[i] = accept(sock_id, (struct sockaddr*) &client[i], &len)) < 0) {
			printf("Error in accepting for client %d\n", i + 1);
			exit(-1);
		}
		cout << "received connection from client " << i + 1 << endl;

	}
	bool ok = true;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		sprintf(input, "%d", i);
		if (send(client_id[i], input, sizeof(input), 0) < 0) {
			ok = false;
		}
	}
	cout << "send confirmation message to both clients" << endl;
	int client_chance = 0;
	while (ok) {
		data_len = recv(client_id[client_chance % 2], data, BUFFER, 0);
		cout << "Received message from client  " << (client_chance % 2) << endl;

		// check if the connection is broken
		if (!data_len || data_len < 0) {
			strcpy(data, "bye");
			send(client_id[(client_chance + 1) % 2], data, sizeof(data), 0);
			break;
		}
		else {
			if (send(client_id[(client_chance + 1) % 2], data, data_len, 0) < 0) {
				break;
			}
			if (strcasecmp(data, "bye") == 0) {
				break;
			}
			cout << "Sending message to client  " << ((client_chance + 1) % 2) << endl;
			client_chance = (client_chance + 1) % 2;
		}
	}

// close the connection
	printf("\nDisconnecting the chat service for this client listening for another client. !!!\n");

	close(client_id[0]);
	close(client_id[1]);

// close the socket
	close(sock_id);

	return 0;
}
