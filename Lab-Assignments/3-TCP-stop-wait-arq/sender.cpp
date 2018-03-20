/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

#define PAYLOADSIZE 1046
using namespace std;
/*
 * error - wrapper for perror
 */

typedef struct segmentHeader
{
	bool sequenceNo;
	bool ackNo;
	int checksum;
	bool lastPacket;
	int length;
	segmentHeader () {
		sequenceNo = 0;
		ackNo = 0;
		checksum = 0;
		lastPacket = true;
		length = 0;
	}
} segmentHeader;

typedef struct segment {
	segmentHeader header;
	char payload[PAYLOADSIZE];
	segment () {
		for (int i = 0; i < PAYLOADSIZE; ++i)
		{
			payload[i] = '\0';
		}
	}
} segment;

int findchecksum(const segmentHeader* packet, const char* payload = NULL) {
	int checksum = 0;
	checksum ^= packet->sequenceNo;
	checksum ^= packet->ackNo;
	checksum ^= packet->lastPacket;
	checksum ^= packet->length;
	if (payload) {
		for (int i = 0; i < PAYLOADSIZE; ++i)
		{
			checksum ^= payload[i];
		}
	}
}

bool checksum(const segmentHeader* packet, const char* payload = NULL) {
	return (packet->checksum == findchecksum(packet, payload));
}

bool recvACK(int sockfd, bool ackState) {
	segmentHeader* ack = new segmentHeader;
	struct sockaddr senderAddr;
	socklen_t senderLen = sizeof(senderAddr);

	if (recvfrom(sockfd, ack, sizeof(*ack), 0, &senderAddr, &senderLen) < 0) {
		cout << "Error in receiving UDP ACK\tReason :: " << std::strerror(errno) << endl;
		return false;
	}
	else if (! checksum(ack)) {
		cout << "Incorrect Checksum for UDP ACK " << endl;
		return false;
	}
	return true;
}

void sendUDPFrame(int sockfd, segment* packet, struct sockaddr* destAddr, bool state) {
	packet->header.checksum = findchecksum(&packet->header, packet->payload);
	while (true) {
		if (sendto(sockfd, packet, sizeof(*packet), 0, destAddr, sizeof(*destAddr)) < 0) {
			cout << "Error in sending UDP frame\tReason :: " << std::strerror(errno) << endl;
		}
		else {
			if (recvACK(sockfd, ~state)) {
				break;
			}
			cout << "Time out +++++++++++++++++++++++++++++++++++++++++++++++++++ " << endl;
		}
	}
}

int main(int argc, char **argv) {
	int sockfd, sender_port , recv_port, n;
	struct sockaddr_in senderAddr;
	struct sockaddr_in recvAddr;

	/* check command line arguments */
	if (argc != 4) {
		fprintf(stderr, "usage: %s <sender_port> <recv_port> <file_path>\n", argv[0]);
		exit(0);
	}
	sender_port = atoi(argv[1]);
	recv_port = atoi(argv[2]);

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		cout << "ERROR opening socket" << endl;
		return 0;
	}

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		cout << "Error in creating timeout for UDP connection " << endl;
		return 0;
	}

	// binding port to socket
	senderAddr.sin_family = AF_INET;
	senderAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	senderAddr.sin_port = htons(sender_port);

	// binding port to client
	recvAddr.sin_family = AF_INET;
	recvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	recvAddr.sin_port = htons(recv_port);

	if (bind(sockfd, (struct sockaddr *)&senderAddr, sizeof(senderAddr)) < 0) {
		perror("cannot bind sender");
		return 0;
	}

	struct stat filestatus;
	stat( argv[ 3 ], &filestatus );

	unsigned int file_size = filestatus.st_size;
	cout << "sending file of " << file_size << " bytes" << endl;

	unsigned int curr_size = 0;
	ifstream fin;
	fin.open(argv[3], ios::binary | ios::in);
	bool sequenceState = 0;

	segment* packet = new segment;

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	char buffer[PAYLOADSIZE];
	int total = 0;
	// string data_p((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
	// int ptr = 0;
	while (!fin.eof()) {

//		strcpy(packet->payload, data_p.substr(ptr , PAYLOADSIZE).c_str() );
		fin.read(packet->payload, PAYLOADSIZE);
//		ptr += PAYLOADSIZE;
		curr_size += PAYLOADSIZE;

		packet->header.sequenceNo = sequenceState;
		packet->header.ackNo = 0;
		packet->header.checksum = 0;

		if (curr_size >= file_size) {
			packet->header.lastPacket = true;
			packet->header.length = file_size - curr_size + PAYLOADSIZE;
		}
		else {
			packet->header.lastPacket = false;
			packet->header.length = PAYLOADSIZE;
		}

		sendUDPFrame(sockfd, packet, (struct sockaddr*) (&recvAddr), sequenceState);
		total++;

		int curr = ((curr_size * 100) / file_size);
		cout << curr << "%\n";
		sequenceState = !sequenceState;
	}
	cout << "total :: " << total << endl;
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;

	close(sockfd);

	return 0;
}