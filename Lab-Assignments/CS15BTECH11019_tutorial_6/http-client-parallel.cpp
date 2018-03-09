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
#include <thread>

using namespace std;

/*
 * error - wrapper for perror
*/

void tcp_connection(int i, int sockfd, int start_range, int end_range) {
	// get file
	char buf[5000];
	ofstream fout;
	string file_name = "./file" + to_string(i) + ".pdf";
	fout.open(file_name.c_str(), ios::binary | ios::out);
	int byte_received;


	int size = end_range - start_range + 1;

	cout << i << " start_range : " << start_range << " , end_range : " << end_range << " , size : " << size << endl;
	string header_string = "GET /files/home/IFS.pdf HTTP/1.1\r\nRange: bytes=" + to_string(start_range) + "-" + to_string(end_range) + "\r\nHost: intranet.iith.ac.in\r\n\r\n";

	const char* header = header_string.c_str();

	if (send(sockfd, header, strlen(header), 0) < 0) {
		cout << "Send failed!! \tReason " << std::strerror(errno) << endl;
		return;
	}
	int byte_count = 0;
	while (byte_count < size) {
		byte_received = recv(sockfd, buf, sizeof(buf) - 1, 0);

		string string_data = string(buf);
		if (byte_received <= 0) {
			return;
		}
		byte_count += byte_received;
		// cout << byte_received << endl;
		int starting = string_data.find("\r\n\r\n");
		if (starting < 0) {
			starting = 0;
		}

		for (int i = starting; i < byte_received; i++)
		{
			fout << buf[i];
		}
	}
	cout << i << " , byte_count :: " << byte_count << endl;
}

int main(int argc, char **argv) {

	struct addrinfo hints, *res;
	int sockfd;

	char buf[5000];
	int byte_count;

	//get host info, make socket and connect it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int no_of_connections = 4;
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

		int index = content.find("Accept-Ranges: ") + strlen("Accept-Ranges: ");
		string size_string = content.substr(index , content.find("\n", index) - index);
		// cout << (size_string == string("bytes")) << endl;
		// if (size_string.compare("bytes\n") != 0) {
		// 	cout << "Multiple TCP is not supported" << endl;
		// 	return 0;
		// }
		// return 0;

		index = content.find("Content-Length: ") + strlen("Content-Length: ");
		size_string = content.substr(index , content.find("\n", index) - index);
		int size = stoi(size_string);
		cout << size << endl;
		thread parallel_tcp[no_of_connections];

		int individual_size = size / no_of_connections + 1;
		int start_range, end_range;

		for (int i = 0; i < no_of_connections; ++i)
		{
			start_range = i * individual_size;
			end_range = (i + 1) * individual_size - 1;
			if (end_range > size) {
				end_range = size - 1;
			}
			parallel_tcp[i] = thread(tcp_connection, i, sockfd, start_range, end_range);
		}

		for (int i = 0; i < no_of_connections; ++i)
		{
			parallel_tcp[i].join();
		}

		std::ofstream of_c("./1b.pdf", std::ios_base::binary);
		for (int i = 0; i < no_of_connections; ++i)
		{
			string file_name = "./file" + to_string(i) + ".pdf";
			std::ifstream if_a(file_name.c_str(), std::ios_base::binary);
			of_c << if_a.rdbuf();
		}
	}

	close(sockfd);

	return 0;
}