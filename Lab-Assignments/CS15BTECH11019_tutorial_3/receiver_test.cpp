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

void sendUDPACK(int sockfd, segmentHeader* ack, struct sockaddr* senderAddr) {
	ack->checksum = findchecksum(ack);
	sendto(sockfd, ack, sizeof(*ack), 0, senderAddr, sizeof(*senderAddr));
}

segment* recvSegment(int sockfd, bool segmentState) {
	segmentHeader* ack = new segmentHeader;
	segment* packet = new segment;

	ack->sequenceNo = 0;
	ack->checksum = 0;
	ack->lastPacket = true;

	struct sockaddr senderAddr;
	socklen_t serverlen =  sizeof(senderAddr);

	while (true) {
		if (recvfrom(sockfd, packet, sizeof(*packet), 0, &senderAddr, &serverlen) < 0) {
			cout << "Error in receiving UDP segment \tReason " << std::strerror(errno) << endl;
		}
		else if (! checksum(&packet->header, packet->payload)) {
			cout << "Incorrect Checksum for UDP segment " << endl;
		}
		else if (packet->header.sequenceNo != segmentState) {
			// dup ACK
			cout << "Error: Incorrect segment number for UDP segment " << endl;
			ack->ackNo = segmentState;
			sendUDPACK(sockfd, ack, &senderAddr);
		}
		else {
			ack->ackNo = ~segmentState;
			sendUDPACK(sockfd, ack, &senderAddr);
			return packet;
		}
	}
}

int main(int argc, char **argv) {
	int sockfd, sender_port , recv_port, n;
	struct sockaddr_in recvAddr;

	char buffer[PAYLOADSIZE];

	/* check command line arguments */
	if (argc != 4) {
		fprintf(stderr, "usage: %s <sender_port> <recv_port> <new_file_path>\n", argv[0]);
		exit(0);
	}
	sender_port = atoi(argv[1]);
	recv_port = atoi(argv[2]);

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		cout << "ERROR opening socket" << endl;
		return 0;
	}

	// binding port to client
	memset((char *)&recvAddr, 0, sizeof(recvAddr));
	recvAddr.sin_family = AF_INET;
	recvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	recvAddr.sin_port = htons(recv_port);

	if (bind(sockfd, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0) {
		perror("cannot bind receiver");
		return 0;
	}

	ofstream fout;
	fout.open(argv[3], ios::binary | ios::out);
	bool sequenceState = 0;
	segment* packet;
	int total = 0;

	while (true) {
		packet = recvSegment(sockfd, sequenceState);
		sequenceState = !sequenceState;
		total++;
		int len = packet->header.length;
		for (int i = 0; i < len; i++)
		{
			fout << packet->payload[i];
		}
		if (packet->header.lastPacket) {
			break;
		}
	}
	close(sockfd);
	return 0;
}