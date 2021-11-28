#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <Windows.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// Change if code changed.
#define SEABIRD_NET_FRAMEWORK_VAR 202110L
/*
New changes:
	- Add erase(), pop_back(), front() and rear() for bytes.
	- Bug fixes of toPost().
*/
#define SEABIRD_NET_FRAMEWORK_SUBVAR 100010L

// Change if STRUCTURE or its PUBLIC BEHAIVOR changed.
// Not include BUG (NOT FEATURE) FIXES.
#define SEABIRD_NET_STRUCTURE_VAR 2

#define SEABIRD_NET_DEBUG 0
#if SEABIRD_NET_DEBUG
#define SEABIRD_NET_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define SEABIRD_NET_DEBUG_PRINT(...) __noop
#endif
#define printf_d(...) SEABIRD_NET_DEBUG_PRINT(__VA_ARGS__)

map<string, string> contentTypes();
string searchTypes(string extension, string def = "text/plain");
long getFileLength(FILE *f);

// Please notices that
// This function may makes empty element.
vector<string> splitLines(const char *data, char spl = '\n', bool firstonly = false, char filter = '\r');

class bytes {
public:
	friend bytes operator + (bytes a, string v);
	friend bytes operator + (bytes a, bytes b);
	friend bytes operator + (bytes a, char b);
	friend bytes operator + (bytes a, const char* b);
	bytes();
	bytes(string b);
	bytes(const char* b);
	bytes(char b);
	bytes(const bytes &other);
	void clear();
	void fill(char c);
	void add(const char * bytes, size_t sz);
	void erase(size_t pos, size_t count = 1);	// Erase char in specified place
	void pop_back(size_t count = 1);			// Remove from back
	char front();
	char rear();
	const char* toCharArray();
	string toString();							// Please notices that string
												// Will search '\0' and ignore informations after it automaticly.
	size_t length();
	bytes operator += (string v);
	bytes operator += (bytes b);
	bytes operator += (char b);
	char& operator [] (size_t pos);
private:
	void realloc(size_t sz);
	char *byte_space;
	size_t len;
};

bytes operator + (bytes a, string v);
bytes operator + (bytes a, bytes b);
bytes operator + (bytes a, char b);
bytes operator + (bytes a, const char* b);

bool operator == (bytes a, bytes b);
bool operator == (bytes a, char b);
bool operator == (bytes a, string b);

// You have to run this before everything!!
WSADATA initalize_socket();

struct path_info {
	vector<string> path;
	map<string, string> exts;
};

struct disp_info {
	string disp_sign;
	map<string, string> attr;
};

struct post_info {
	string boundary;
	map<string, string> attr;
	bytes content;

	void saveContent(FILE *hnd);
	disp_info toDispInfo();
};

struct content_info {
	string ctype;
	string boundary;
};

struct http_recv {
	string proto_ver;			// 1.0 or 1.1
	string process;				// methods, e.g. GET, POST, PUT
	string path;				// e.g. "/", "/index.html"
	map<string, string> attr;	// Attributes
	bytes content;				// Message body

	path_info toPaths();		// To split path
	content_info toCType();		// To Content-Type information
	vector<post_info> toPost();	// To POST informations
};

struct http_send {
	string proto_ver;			// The same as http_recv
	int codeid;					// e.g. 200. HTTP codes.
	string code_info;			// e.g. 200 OK; 404 Not Found...
	map<string, string> attr;
	bytes content;

	void loadContent(FILE *hnd);
								// Load content from a file.
	bytes toSender(bool autolen = true);			// Returns sendable info.
};

#define RCV_DEFAULT 4096

// Socket for server.
class ssocket {
public:
	ssocket();
	ssocket(SOCKET s);
	ssocket(int port, int rcvsz = RCV_DEFAULT);
	bool binds(int port);
	bool listens(int backlog = 5);
	bool accepts();
	bool vaild();
	bool accept_vaild();
	// These functions requires accepts():
	http_recv receive();		// Before call this call accepts().
	bool sends(bytes data);
	void end_accept();
	void end();
private:
	void sock_init(int rcvsz = RCV_DEFAULT);
	bytes raw_receive();		// It can keep receiving

	SOCKET s,ace; // ace = Accepted socket
	sockaddr acc; // acc = Accepted socket address
	char *recv_buf;
	int rcbsz, last_receive;
	bool acc_errored,errored;
};