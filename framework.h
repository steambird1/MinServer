#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define CRTDBG_MAP_ALLOC
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <Windows.h>
#include <crtdbg.h>
#include <thread>
#include <mutex>
using namespace std;

#pragma comment(lib, "ws2_32.lib")
#ifndef __cplusplus
#error Seabird Network Library can't be used in C mode
#endif
#define SEABIRD_NET_FRAMEWORK

// Change if code changed.
#define SEABIRD_NET_FRAMEWORK_VER 202201L

// Change if STRUCTURE or its PUBLIC BEHAIVOR changed.
// Not include BUG (NOT FEATURE) FIXES.
#define SEABIRD_NET_STRUCTURE_VER 2
#define SEABIRD_NET_STRUCTURE_SUBVER 2

#define SEABIRD_NET_DEBUG 0
#if SEABIRD_NET_DEBUG == 1
#define SEABIRD_NET_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#if SEABIRD_NET_DEBUG == 2
#include "debugs.h"
#endif
#define SEABIRD_NET_DEBUG_PRINT(...) __noop
#endif
#define printf_d(...) SEABIRD_NET_DEBUG_PRINT(__VA_ARGS__)

#define DEPRECATED(text) __declspec(deprecated(text))

#ifndef SEABIRD_NET_NO_MEM_DEPRECATE
#define MEM_DEPRECATE(cons) DEPRECATED("Too much memory usage in this function. consider directly use: "#cons)
#endif

#ifndef SEABIRD_NET_NO_THREAD_DEPRECATE
#define THREAD_DEPRECATE(cons) DEPRECATED("This version doesn't support multithreading. consider using "#cons)
#endif

map<string, string> contentTypes();
string searchTypes(string extension, string def = "text/plain");
long getFileLength(FILE *f);

// Please notices that
// This function may makes empty element.
vector<string> splitLines(const char *data, char spl = '\n', bool firstonly = false, char filter = '\r');

template <typename TKey, typename TValue>
class t_safe_table;

// Prepare for threading.
template <typename Ty>
class t_safe {
public:
	template <typename TKey, typename TValue>
	friend map<TKey,TValue>& t_safe_table<TKey,TValue>::unsafe_get();
	t_safe() {

	}
	Ty& get() {
		while (true) {
			try {
				m.lock();
				return this->data;
			}
			catch (...) {
				// A fail lock causes exception
			}
		}
	}
	bool try_lock() {
		return m.try_lock();
	}
	void unlock() {
		m.unlock;
	}
private:
	Ty data;
	mutex m;
};

template <typename TKey, typename TValue>
class t_safe_table {
public:
	using mapdata = map<TKey, TValue>;
	t_safe_table() {
	
	}
	size_t size() {
		return v.get().size();
	}
	bool count(TKey member) {
		return v.get().count(member);
	}
	TValue& operator [](TKey value) {
		return v.get()[value];
	}
	mapdata& unsafe_get() {
		return v.data;
	}
	bool try_lock() {
		return v.try_lock();
	}
	void unlock() {
		v.unlock();
	}
private:
	t_safe<mapdata> v;
};

// Pre-declaration
class sscoket;
class bytes;

class bytes {
public:
	friend bytes operator + (const bytes& a, string v);
	friend bytes operator + (const bytes& a, const bytes& b);
	friend bytes operator + (const bytes& a, char b);
	friend bytes operator + (const bytes& a, const char* b);
	friend bool operator == (const bytes &a, const bytes &b);
	friend bool operator == (const bytes &a, char b);
	friend bool operator == (const bytes &a, string b);
//	friend bool ssocket::sends(http_send& sender);
	bytes();
	bytes(string b);
	bytes(const char* b);
	bytes(char b);
	bytes(const bytes &other);
	~bytes();
	void release();
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
	void operator += (string v);
	void operator += (const bytes& b);
	void operator += (char b);
	char& operator [] (size_t pos);
	operator string();
	static int decst;
	// Fuck optimize
	//~bytes() = delete;
private:
	void realloc(size_t sz);
	char *byte_space;
	size_t len;
};

bytes operator + (const bytes& a, string v);
bytes operator + (const bytes& a, const bytes& b);
bytes operator + (const bytes& a, char b);
bytes operator + (const bytes& a, const char* b);

bool operator == (const bytes& a, const bytes& b);
bool operator == (const bytes& a, char b);
bool operator == (const bytes& a, string b);

#define RCV_DEFAULT 8192

#pragma region(SSocket Extended Types)
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
	bytes path;				// e.g. "/", "/index.html"
	map<string, string> attr;	// Attributes
	bytes content;				// Message body

	void release();
	path_info toPaths();		// To split path
	content_info toCType();		// To Content-Type information
	vector<post_info> toPost();	// To POST informations
};

struct http_send {
	bool raw_sending = false;
	bytes raw_send;				// Raw sending, not through toSender()-like

	string proto_ver;			// The same as http_recv
	int codeid;					// e.g. 200. HTTP codes.
	string code_info;			// e.g. 200 OK; 404 Not Found...
	map<string, string> attr;
	bytes content;

	void loadContent(FILE *hnd);
	// Load content from a file.
	// Deprecated
	MEM_DEPRECATE("ssocket::sends(sender&)")
	inline bytes toSender(bool autolen = true) {
		bytes b = proto_ver + " " + to_string(codeid) + " " + code_info + "\n";
		if (autolen) attr["Content-Length"] = to_string(this->content.length());
		//	 for (auto i = attr.begin(); i != attr.end(); i++)
		//		 b += (i->first + ": " + i->second) + "\n";
		for (auto &i : attr) {
			b += (i.first + ": " + i.second + "\n");
		}
		b += '\n';
		b += content;
		return move(b);
	} // Returns sendable info.
};
#pragma endregion

class ssocket {
public:

	class acceptor {
	public:
		acceptor(SOCKET a, sockaddr_in sa, int recv_buf = RCV_DEFAULT);
		void receive(http_recv &h);
		bool sends(http_send& sender);
		void end_accept();
		bool accept_vaild();
		bytes& get_prev();
		void release_prev();
		const char* get_paddr();
	private:
		bytes raw_receive();
		SOCKET ace;
		sockaddr_in acc;
		bytes prev_recv;
		char *recv_buf;
		int rcbsz, last_receive;
		bool acc_errored;
	};

	ssocket();
	ssocket(SOCKET s);
	ssocket(int port, int rcvsz = RCV_DEFAULT);
	bool binds(int port);
	bool listens(int backlog = 5);
	THREAD_DEPRECATE("accepts() with function objects.")
	bool accepts();
	bool accepts(function<http_send(http_recv&, bytes&, const char*)> acceptor, function<void(void)> runner = []() {});
	bool vaild();
	THREAD_DEPRECATE("accepts()")
	bool accept_vaild();
	// Deprecated
	MEM_DEPRECATE("ssocket::receive(receiver&)")
	http_recv receive();		// Before call this call accepts().
	THREAD_DEPRECATE("accepts() with given paramaters")
	void receive(http_recv &h); // To save memory
	THREAD_DEPRECATE("accepts() with given paramaters")
	bool sends(bytes& data);
	THREAD_DEPRECATE("accepts() with given paramaters")
	bool sends(http_send& sender);
	THREAD_DEPRECATE("accepts()")
	void end_accept();
	void end();
	bytes& get_prev();
	void release_prev();
	const char* get_paddr();
private:
	void sock_init(int rcvsz = RCV_DEFAULT);
	bytes raw_receive();		// It can keep receiving

	bytes prev_recv;
	SOCKET s, ace; // ace = Accepted socket
	sockaddr_in acc; // acc = Accepted socket address
	char *recv_buf;
	int rcbsz, last_receive;
	bool acc_errored, errored;
};

// You have to run this before everything!!
WSADATA initalize_socket();

// Tools

// Hex numbers -> Dec numbers
int hex2dec(string hex);
// Resolve HTTP marks (e.g. %20 -> ' ').
bytes resolveHTTPSymbols(string s);