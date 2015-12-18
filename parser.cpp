#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

using namespace std;

class Parser {
private:
	static const int START = 0;
	static const int ARRAY_SIZE = 1;
	static const int ARRAY_BEGIN = 2;
	static const int ARRAY_BODY = 3;
	static const int STRING_SIZE = 4;
	static const int STRING_BEGIN = 5;
	static const int STRING_BODY = 6;
	static const int STRING_END = 7;
	static const int OK = 8;
	static const int ERROR = -1;
public:
	Parser() {}

	int bulktos(string request, vector<string>& vec) {
		int state = START;
		int array_size = 0;
		int str_size;
		int sleft;
		int aleft;
		int i = 0;
		string str;
		while (1) {
			char symbol = request[i];
			switch(state) {
				case START:
					if (symbol == '*') {
						state = ARRAY_SIZE;
					} else {
						return ERROR;
					}
					break;
				case ARRAY_SIZE:
					if (symbol >= '0' && symbol <= '9') {
						int digit = symbol - '0';
						array_size = array_size * 10 + digit;
					} else if (symbol == '\r') {
						state = ARRAY_BEGIN;
						aleft = array_size;
						// cerr << "array size is " << array_size << endl;
					} else {
						return ERROR;
					}
					break;
				case ARRAY_BEGIN:
					if (symbol == '\n') {
						state = ARRAY_BODY;
					} else {
						return ERROR;
					}
					break;
				case ARRAY_BODY:
					if (symbol == '$' && aleft != 0) {
						state = STRING_SIZE;
						--aleft;
						str_size = 0;
						str = "";
					} else {
						return ERROR;
					}
					break;
				case STRING_SIZE:
					if (symbol >= '0' && symbol <= '9') {
						int digit = symbol - '0';
						str_size = str_size * 10 + digit;
					} else if (symbol == '\r') {
						sleft = str_size;
						state = STRING_BEGIN;
						// cerr << "string size is " << str_size << endl;
					} else {
						return ERROR;
					}
					break;
				case STRING_BEGIN:
					if (symbol == '\n') {
						state = STRING_BODY;
					} else {
						return ERROR;
					}
					break;
				case STRING_BODY:
					if (symbol == '\r' && sleft == 0) {
						state = STRING_END;
					} else if (sleft > 0) {
						str.push_back(symbol);
						--sleft;
					} else {
						return ERROR;
					}
					break;
				case STRING_END:
					if (symbol == '\n' && aleft != 0) {
						// cout << "str: " << str << endl;
						vec.push_back(str);
						state = ARRAY_BODY;
					} else if (symbol == '\n' && aleft == 0) {
						cout << "string parsed" << endl;
						vec.push_back(str);
						return OK;
					} else {
						return ERROR;
					}
					break;
				default:
					break;

			}
			++i;
		}
		return state;
	}

	vector<string> parse(string request) {
		vector<string> vec;
		int status = bulktos(request, vec);
		cout << "status: " << status << endl;
		if (status != OK) {
			vec.clear();
			return vec;
		}
		return vec;
	}


	string stobulk(string message) {
	return "$" + to_string(message.length()) + "\r\n" + message + "\r\n";
	}

	string etobulk(string message) {
		return "-" + message + "\r\n";
	}

	string ok() {
		return "+OK\r\n";
	}

	string nil() {
		return "$-1\r\n";
	}
};
