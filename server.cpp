#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string.h>
#include <algorithm>
#include <set>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "parser.cpp"
#include "database.cpp"

using namespace std;

class Server {
private:
	int listener;
    struct sockaddr_in addr;
    char buf[1024];
    int bytes_read;
    set<int> clients;
    timeval timeout;

    Parser parser;
    Database database;

    bool file_exists() {
    	ifstream in("/tmp/redis.pid");
    	bool status = in.good() ? true : false;
    	in.close();
    	return status;
    }

    int handle_request(int client) {
		bytes_read = recv(client, buf, 1024, 0);
		if (bytes_read <= 0) {
			return 1;
		}

		vector<string> request = parser.parse(string(buf));

		if (request.size() == 2 && strcasecmp(request[0].c_str(), "GET") == 0) {
			string value = database.get(request[1]);
			if (value != "") {
				value = parser.stobulk(value);
				send(client, value.c_str(), value.length(), 0);
			} else {
				string nil = parser.nil();
				send(client, nil.c_str(), nil.length(), 0);
			}
		} else if (request.size() == 3 && strcasecmp(request[0].c_str(), "SET") == 0) {
			database.set(request[1], request[2]);
			string ok = parser.ok();
			send(client, ok.c_str(), ok.length(), 0);
		} else {
			cout << "Wrong request" << endl;
			string error_message = parser.etobulk("Error");
			send(client, error_message.c_str(), error_message.length(), 0);
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

		fcntl(listener, F_SETFL, O_NONBLOCK);

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			perror("Bind error");
			exit(0);
		}
		listen(listener, 5);
		clients.clear();
		cout << "Init success " << "port: " << port << endl;
	}

	void run() {
		pid_t pid = fork();
		if (pid == -1) {
			perror("Daemon");
		} else if (pid == 0) {
			setsid();
			while (true) {
				fd_set readset;
				FD_ZERO(&readset);
				FD_SET(listener, &readset);

				for (set<int>::iterator it = clients.begin(); it != clients.end(); ++it) {
					FD_SET(*it, &readset);
				}

				timeout.tv_sec = 15;
				timeout.tv_usec = 0;

				int mx = max(listener, *max_element(clients.begin(), clients.end()));
				if (select(mx + 1, &readset, NULL, NULL, &timeout) <= 0) {
					continue;
				}

				if (FD_ISSET(listener, &readset)) {
					int sock = accept(listener, NULL, NULL);
					cout << "socket: " << sock << endl;
					if (sock < 0) {
						perror("Accept error");
						exit(0);
					}
					fcntl(sock, F_SETFL, O_NONBLOCK);
					clients.insert(sock);
				}

				set<int> need_to_erase;
				for (set<int>::iterator it = clients.begin(); it != clients.end(); ++it) {
					if (FD_ISSET(*it, &readset)) {
						int status = handle_request(*it);
						if (status == 1) {
							need_to_erase.insert(*it);
						}
					}
				}

				for (set<int>::iterator it = need_to_erase.begin(); it != need_to_erase.end(); ++it){
					clients.erase(*it);
				}
			}
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