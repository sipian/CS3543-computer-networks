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
#include <thread>
#include <atomic>

#define PORT 10000
#define ERROR -1
#define MAX_CLIENTS 2
#define BUFFER 1024
using namespace std;

std::atomic_int isConnectionOpen = ATOMIC_VAR_INIT(1);


void receiveClient(int my_id, int my_sock_id, int other_sock_id) {

	char input[BUFFER];
	int data_len;
	sprintf(input, "%d", my_id);
	if (send(my_sock_id, input, sizeof(input), 0) < 0) {
		printf("Error in Sending Id to client %d\n", my_id + 1);
		isConnectionOpen = 0;
		return;
	}

	cout << "sent confirmation message to client " << my_id + 1 << endl;

	while (isConnectionOpen) {

		data_len = recv(my_sock_id, input, BUFFER, 0);
		cout << "Received message from client  " << my_id + 1 << endl;

		// check if the connection is broken or wants to close
		if (!data_len || data_len < 0 || strcasecmp(input, "bye") == 0) {
			strcpy(input, "bye");
			send(other_sock_id, input, sizeof(input), 0);
			isConnectionOpen = 0;
			break;
		}
		else {
			if (send(other_sock_id, input, data_len, 0) < 0) {
				isConnectionOpen = 0;
				break;
			}
			cout << "Sent message to client  " << my_id + 1 << endl;
		}
	}

}


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
	thread clientThreads[MAX_CLIENTS];

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if ((client_id[i] = accept(sock_id, (struct sockaddr*) &client[i], &len)) < 0) {
			printf("Error in accepting for client %d\n", i + 1);
			exit(-1);
		}
		cout << "received connection from client " << i + 1 << endl;
	}

	thread client1 = thread(receiveClient, 0, client_id[0], client_id[1]);
	thread client2 = thread(receiveClient, 1, client_id[1], client_id[0]);

	client1.join();
	client2.join();

	// close the connection
	printf("\nDisconnecting the chat service for this client listening for another client. !!!\n");

	// close the socket
	close(client_id[0]);
	close(client_id[1]);
	close(sock_id);

	return 0;
}
