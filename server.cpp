#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "parser.cpp"
#include "database.cpp"

using namespace std;

class Server {
private:
	int sock;
	int listener;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;

    Parser parser;
    Database database;

    bool file_exists() {
    	ifstream in("/tmp/redis.pid");
    	bool status = in.good() ? true : false;
    	in.close();
    	return status;
    }

    int handle_request() {
		bytes_read = recv(sock, buf, 1024, 0);
		if (bytes_read <= 0) {
			return 1;
		}

		vector<string> request = parser.parse(string(buf));

		if (request.size() == 2 && strcasecmp(request[0].c_str(), "GET") == 0) {
			string value = database.get(request[1]);
			if (value != "") {
				value = parser.stobulk(value);
				send(sock, value.c_str(), value.length(), 0);
			} else {
				string nil = parser.nil();
				send(sock, nil.c_str(), nil.length(), 0);
			}
		} else if (request.size() == 3 && strcasecmp(request[0].c_str(), "SET") == 0) {
			database.set(request[1], request[2]);
			string ok = parser.ok();
			send(sock, ok.c_str(), ok.length(), 0);
		} else {
			cout << "Wrong request" << endl;
			string error_message = parser.etobulk("Error");
			send(sock, error_message.c_str(), error_message.length(), 0);
		}
		return 0;
    }

public:
	Server() {};

	static void tear_down(int sig) {
		remove("/tmp/redis.pid");
		exit(0);
	}

	void init(int port) {
		signal(SIGUSR1, tear_down);

		if (file_exists()) {
			perror("Server already is running");
			exit(0);
		}

		listener = socket(AF_INET, SOCK_STREAM, 0);
		if (listener < 0) {
			perror("Socket error");
			exit(0);
		}
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("Bind error");
			exit(0);
		}
		listen(listener, 1);
		cout << "Init success " << "port: " << port << endl;
	}

	void run() {
		pid_t pid = fork();
		if (pid == -1) {
			perror("Daemon");
		} else if (pid == 0) {
			cout << "I'm server epta!" << endl;
			setsid();
			sock = accept(listener, NULL, NULL);
			cout << sock << endl;
			if (sock < 0) {
				perror("Accept error");
				exit(0);
			}

			int status = 0;
			while (status == 0) {
				status = handle_request();
			}
			close(sock);
		} else {
			ofstream out("/tmp/redis.pid");
			out << pid;
			out.close();
			cout << "Server launched" << endl;
			return;
		}
	}

	void stop() {
		pid_t pid;
		ifstream in("/tmp/redis.pid");
		if (!in.good()) {
			perror("Server isn't launched");
			exit(0);
		}
		in >> pid;
		in.close();
		kill(pid, SIGUSR1);
		cout << "Server was stopped" << endl;
		return;
	}

};