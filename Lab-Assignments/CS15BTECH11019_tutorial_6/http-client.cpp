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
#include <vector>
#include <regex>
using namespace std;

/*
 * error - wrapper for perror
*/

int main(int argc, char **argv) {

	struct addrinfo hints, *res;
	int sockfd;

	char buf[5000];
	int byte_count;

	//get host info, make socket and connect it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo("intranet.iith.ac.in", "80", &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		printf("Not Connected!\n");
		exit(0);
	}
	else {
		printf("Connected Successfully!\n");
		cout << endl;
	}
	char header[] = "HEAD /files/home/IFS.pdf HTTP/1.1\r\nHost: intranet.iith.ac.in\r\n\r\n";
	if (send(sockfd, header, strlen(header), 0) < 0) {
		cout << "Send failed!! \tReason " << std::strerror(errno) << endl;
		return 0;
	}
	else {
		byte_count = recv(sockfd, buf, sizeof(buf) - 1, 0);
		if (byte_count < 0 ) {
			cout << "Recv failed.!! " << endl;
			return 0;
		}
		buf[byte_count] = '\0';
		printf("recv()'d %d bytes of data in buf\n", byte_count);
		cout << buf << endl;
		string content(buf);

		int index = content.find("Content-Length: ") + strlen("Content-Length: ");
		string size_string = content.substr(index , content.find("\n", index) - index);
		int size = stoi(size_string);
		cout << size << endl;

		// get file
		ofstream fout;
		fout.open("./file.pdf", ios::binary | ios::out);
		int byte_received;

		char header[] = "GET /files/home/IFS.pdf HTTP/1.1\r\nHost: intranet.iith.ac.in\r\n\r\n";
		if (send(sockfd, header, strlen(header), 0) < 0) {
			cout << "Send failed!! \tReason " << std::strerror(errno) << endl;
			return 0;
		}
		int byte_count = 0;
		while (byte_count < size) {
			byte_received = recv(sockfd, buf, sizeof(buf), 0);
			string string_data = string(buf);
			byte_count += byte_received;

			if (byte_received < 0 ) {
				cout << "Recv failed.!! " << endl;
				return 0;
			}
			cout << byte_received << endl;
			int starting = string_data.find("\r\n\r\n") + 4;
			if(byte_count != byte_received) {
				starting = 0;
			}

			for (int i = starting; i < byte_received; i++)
			{
				fout << buf[i];
			}
		}
	}


	close(sockfd);

	return 0;
}
