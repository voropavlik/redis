#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

class Database {
private:
	unordered_map<string, string> database;
public:
	void set(string key, string value) {
		database.insert(make_pair(key, value));
	}

	string get(string key) {
		cerr << key << endl;
		if (database.find(key) != database.end()) {
			return database.find(key)->second;
		} else {
			return "";
		}
	}
};