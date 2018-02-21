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
#include <vector>
#include <thread>
#include <mutex>

#define PAYLOADSIZE 1046
using namespace std;
using namespace chrono;

/*
 * error - wrapper for perror
*/

typedef struct segmentHeader
{
	int sequenceNo;
	int ackNo;
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


int base = 0;
int nextseqnum = 0;
int windowSize = 10;
std::vector<segment* > file_data;
int n_packets = 0;
std::mutex mtx;           // mutex for critical section

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
	return checksum;
}

bool checksum(const segmentHeader* packet, const char* payload = NULL) {
	return (packet->checksum == findchecksum(packet, payload));
}


void rdt_rcv(int sockfd, struct sockaddr* destAddr) {
	segmentHeader* ack = new segmentHeader;
	segment* packet;
	struct sockaddr senderAddr;
	socklen_t senderLen = sizeof(senderAddr);

	while (1) {
		if (recvfrom(sockfd, ack, sizeof(*ack), 0, &senderAddr, &senderLen) < 0) {
			// timeout
			int base_local = base;
			int nextseqnum_local = nextseqnum;
			cout << "Failed Transmission - base " << base_local << " - nextseqnum " << nextseqnum_local << endl;
			for (int i = base_local; i < nextseqnum_local ; i++) {
				packet = file_data[i];
				cout << "-- sending " << packet->header.sequenceNo << endl;
				if (sendto(sockfd, packet, sizeof(*packet), 0, destAddr, sizeof(*destAddr)) < 0) {
					cout << "Error in sending UDP frame\tReason :: " << std::strerror(errno) << endl;
				}
			}
		}
		else if (! checksum(ack)) {
			cout << "Incorrect Checksum for UDP segment " << endl;
		}
		else {
			cout << "++ received " << ack->ackNo << endl;
			base = ack->ackNo;
			//cout << "** received " << base << " -- " << file_data[base]->header.sequenceNo  << endl;
			if (base >= (n_packets-1)) {
				break;
			}
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
	tv.tv_usec = 100000;
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

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	// save data
	int counter = 0;
	while (!fin.eof()) {
		segment* packet = new segment;
		packet->header.sequenceNo = counter;
		counter++;
		packet->header.ackNo = 0;
		fin.read(packet->payload, PAYLOADSIZE);
		curr_size += PAYLOADSIZE;

		if (curr_size >= file_size) {
			packet->header.lastPacket = true;
			packet->header.length = file_size - curr_size + PAYLOADSIZE;
		}
		else {
			packet->header.lastPacket = false;
			packet->header.length = PAYLOADSIZE;
		}
		packet->header.checksum = findchecksum(&packet->header, packet->payload);
		file_data.push_back(packet);
	}

	n_packets = file_data.size();
	cout << "READ full file " << n_packets << endl;
	segment* packet;
	struct sockaddr* destAddr = (struct sockaddr*) (&recvAddr);
	thread recv_thread = thread(rdt_rcv, sockfd, destAddr);
	while (base < n_packets-1) {
		if (nextseqnum < base + windowSize && nextseqnum < n_packets) {
			packet = file_data[nextseqnum];
			cout << "-- sending " << packet->header.sequenceNo << endl;
			if (sendto(sockfd, packet, sizeof(*packet), 0, destAddr, sizeof(*destAddr)) < 0) {
				cout << "Error in sending UDP frame\tReason :: " << std::strerror(errno) << endl;
			}
			nextseqnum++;
		}
	}

	recv_thread.join();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;

	close(sockfd);

	return 0;
}