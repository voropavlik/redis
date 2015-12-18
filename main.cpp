#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "server.cpp"

int main(int argc, char** argv) {
	Server redis_server;
	if (argc < 2) {
		cout << "Wrong command" << endl;
	} else if (strcmp(argv[1], "start") == 0) {
		redis_server.init(6379);
		redis_server.run();
	} else if (strcmp(argv[1], "stop") == 0) {
		redis_server.stop();
	}
	return 0;
}