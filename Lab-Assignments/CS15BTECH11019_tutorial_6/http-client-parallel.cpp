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
#include <chrono>

using namespace std;

/*
 * error - wrapper for perror
*/

void tcp_connection(string hostname, string pdf, int i, int start_range, int end_range) {
	int sockfd;
	struct addrinfo hints, *res;

	int size = end_range - start_range + 1;

	//get host info, make socket and connect it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(hostname.c_str(), "80", &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		printf("Not Connected!\n");
		exit(0);
	}
	else {
		printf("Connected Successfully!\n");
		cout << endl;
	}

	// get file
	ofstream fout;
	string file_name = "./file" + to_string(i) + ".pdf";
	fout.open(file_name.c_str(), ios::binary | ios::out);

	cout << i << " start_range : " << start_range << " , end_range : " << end_range << " , size : " << size << endl;
	string header_string = "GET " + pdf + " HTTP/1.1\r\nRange: bytes=" + to_string(start_range) + "-" + to_string(end_range) + "\r\nHost: " + hostname + "\r\n\r\n";

	if (send(sockfd, header_string.c_str(), header_string.length(), 0) < 0) {
		cout << "Send failed!! \tReason " << std::strerror(errno) << endl;
		return;
	}
	char buf[5000];
	int byte_count = 0;
	int byte_received = 0;

	while (byte_count < size) {
		byte_received = recv(sockfd, buf, sizeof(buf) - 1, 0);

		string string_data = string(buf);
		if (byte_received <= 0) {
			return;
		}
		byte_count += byte_received;
		int starting = string_data.find("\r\n\r\n");
		if (starting < 0) {
			starting = 0;
		}
		else {
			starting += 4;
		}

		for (int i = starting; i < byte_received; i++)
		{
			fout << buf[i];
		}
	}
	cout << i << " , byte_count :: " << byte_count << endl;
}

int main(int argc, char **argv) {

	if (argc != 4) {
		cout << "Incorrect arguements. Give <hostname> <file> <no_of_connections>";
		return 0;
	}
	string hostname(argv[1]);
	string filename(argv[2]);
	struct addrinfo hints, *res;
	int sockfd;

	//get host info, make socket and connect it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int no_of_connections = atoi(argv[3]);
	getaddrinfo(argv[1], "80", &hints, &res);

	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		printf("Not Connected!\n");
		exit(0);
	}
	else {
		printf("Connected Successfully!\n");
		cout << endl;
	}

	string header = "HEAD " + filename + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";

	if (send(sockfd, header.c_str(), header.length(), 0) < 0) {
		cout << "Send failed!! \tReason " << std::strerror(errno) << endl;
		return 0;
	}
	else {
		char buf[5000];
		int byte_count;
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
		cout << "File size in bytes :: " << size << endl;
		thread parallel_tcp[no_of_connections];

		int individual_size = size / no_of_connections + 1;
		int start_range, end_range;
		std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

		for (int i = 0; i < no_of_connections; ++i)
		{
			start_range = i * individual_size;
			end_range = (i + 1) * individual_size - 1;
			if (end_range > size) {
				end_range = size - 1;
			}
			parallel_tcp[i] = thread(tcp_connection, hostname, filename, i, start_range, end_range);
		}

		for (int i = 0; i < no_of_connections; ++i)
		{
			parallel_tcp[i].join();
		}

		std::ofstream of_c("./result.pdf", std::ios_base::binary);
		for (int i = 0; i < no_of_connections; ++i)
		{
			string file_name = "./file" + to_string(i) + ".pdf";
			std::ifstream if_a(file_name.c_str(), std::ios_base::binary);
			of_c << if_a.rdbuf();
			remove(file_name.c_str());
		}
		std::chrono::steady_clock::time_point finish_time = std::chrono::steady_clock::now();
		std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time).count() << "ms" << std::endl;
	}

	close(sockfd);

	return 0;
}