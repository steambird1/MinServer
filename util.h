#pragma once
#include <Windows.h>
#include <cstdio>
#include <ctime>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include "sfs_util.h"
//#include "file_object.h"
#define MINSERVER_DEBUG 0
#if MINSERVER_DEBUG == 4
#include "debugs.h"
#endif
//#include "c_framework.h"
using namespace std;

// For some reasons
#define MINSERVER_VER "3.1a"


string getExt(string cwtemp) {
	string exte = cwtemp;
	for (size_t i = cwtemp.length() - 1; i >= 0; i--) {
		if (cwtemp[i] == '.') {
			exte = exte.substr(i);
			break;
		}
	}
	return exte;
}

// Default ones
vector<string> defiles = { "", "index.html","index.htm" };
map<string, string> ctypes = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };

string defaultType = "text/plain";

string findType(string fn) {
	string ext = getExt(fn);
	if (!ctypes.count(ext))
		return defaultType;
	return ctypes[ext];
}

int random_s(void) {
	static bool firstrun = true;
	if (firstrun) {
		srand(time(NULL));
		firstrun = false;
	}
	return rand();
}

int random(int bitm = 16) {
	int n = 0;
	for (int i = 0; i <= bitm; i++) {
		if (random_s() % 2) n |= (1 << i);
	}
	return n;
}

string makeTemp(string end_suffix = "") {
	string s;
	do {
		s = string(getenv("temp")) + "\\" + to_string(random()) + end_suffix;
	} while (fileExists(s));
	return s;
}

const bytes not_found_d = "<html><head><title>Page not found - 404</title></head><body><h1>404 Not found</h1><p>Requested page not found on this server.</p><hr /><p>MinServer " MINSERVER_VER "</p></body></html>";
const bytes not_supported_d = "<html><head><title>Not Implemented - 501</title></head><body><h1>501 Not Implemented</h1><p>Request not implemented by server.</p><hr /><p>MinServer " MINSERVER_VER "</p></body></html>";
const bytes no_perm_d = "<html><head><title>Forbidden - 403</title></head><body><h1>403 Forbidden</h1><p>You don't have permission to view this page. It might because:</p><ul><li>Your administrator doesn't give you the permission to view this page.</li><li>There are too many redirections.</li><li>Your IP is banned (if you can't view any page).</li></ul><hr /><p>MinServer " MINSERVER_VER "</p></body></html>";

// Uses for VBS.
const char* no_ret_d = "<html><head><title>Internal Server Error - 500</title></head><body><h1>500 Internal Server Error</h1><p>Background process did not return any content.</p><hr /><p>MinServer " MINSERVER_VER "</p></body></html>";
const char* err_ret_ndisp = "Error message set as not publish. Use --always-display-err or local machine to display error.";
// !! It requires sprintf() with error information !!
const char* err_ret_d = "<html><head><title>Internal Server Error - 500</title></head><body><h1>500 Internal Server Error</h1><p>Background process reported an error: </p>%s<hr /><p>MinServer " MINSERVER_VER "</p></body></html>";

bytes not_found_c = not_found_d, not_supported_c = not_supported_d, no_perm_c = no_perm_d;

// !! It requires sprintf() with redirection information !!
string ok = "<html><head><title>OK - 200</title></head><body><h1>OK</h1><p>Requested operation completed successfully.</p>%s<hr /><p>MinServer " MINSERVER_VER "</p></body></html>";
// !! It requires sprintf() for the link and ENCODED LINK and sprintf to 'ok' !!
string redirect = "<p>You are going to be redirect. If this page does not redirect automaticly, click <a href=\"%s\">here</a>.</p><script>window.location.href = \"%s\"</script>";

// Controller class.
class uidctrl {
public:
	// Not check!
	static void force_request(int uid, int tk) {
		if (uid_to_token.count(uid)) {
			token_to_uid.erase(uid_to_token[uid]);
		}
		token_to_uid[tk] = uid;
		uid_to_token[uid] = tk;
	}
	// Normal request
	static int request(int uid) {
		int tk;
		do {
			tk = random();
		} while (token_to_uid.count(tk));
		force_request(uid, tk);
		return tk;
	}
	static bool vaild(int token) {
		if (!token_to_uid.count(token)) return false;
		if (!uid_to_token.count(token_to_uid[token])) return false;
		return true;
	}
	static int uidof(int token) {
		if (!token_to_uid.count(token)) return -1;
		return token_to_uid[token];
	}
	static void release(int token) {
		uid_to_token.erase(token_to_uid[token]);
		token_to_uid.erase(token);
	}
	static int size() {
		return token_to_uid.size();
	}
	static auto getmap() {
		return token_to_uid;
	}
private:
	static map<int, int> token_to_uid, uid_to_token;
};

// Pre-declaration for memory space
map<int, int> uidctrl::token_to_uid, uidctrl::uid_to_token;

// Permission showing:

// 421
// RWA
map<char, int> fopens_perm = { {'r', 4}, {'w', 2}, {'a', 1} };

int getPermOf(string expr) {
	if (!expr.length()) return 0;
	int res = fopens_perm[expr[0]];
	if (expr.length() == 1) return res;
	else {
		if (expr[1] == '+') {
			switch (expr[0]) {
			case 'r':
				res |= 2;
				break;
			case 'w': case 'a':
				res |= 4;
				break;
			default:
				break;
			}
		}
	}
	return res;
}

string dec2hex(int n) {
	string res = "";
	while (n != 0) {
		int x = n % 16;
		if (x < 10) {
			res = char(x + '0') + res;
		}
		else {
			res = char(x + 'A' - 10) + res;
		}
		n /= 16;
	}
	return res;
}

string dec2hexw(int n) {
	string s = dec2hex(n);
	char tmp[8];
	sprintf(tmp, "%02s", s.c_str());
	return tmp;
}

inline int permMatch(int req, int cur) {
	return (req & cur) == req;
}

#define decodeHTMLBytes resolveHTTPSymbols

// encode. e.g. '0' -> '\x30'.
string encodeBytes(bytes b) {
	string res = "";
	for (size_t i = 0; i < b.length(); i++) {
		res += "\\x" + dec2hexw(b[i]);
	}
	return res;
}

string sRemovingEOL(string s) {
	string t = s;
	while (t[t.length() - 1] == '\n') t.pop_back();
	return move(t);
}

string sCurrDir(string s = "") {
	char buf3[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buf3);
	if (s.length()) {
		if (s[0] == '/' || s[0] == '\\') s.erase(s.begin());
	}
	return string(buf3) + "\\" + s;
}

string sRemovingQuotes(string s) {
	string t = s;
	if (t[0] == '"' || t[0] == '\'') t = t.substr(1);
	if (t[t.length() - 1] == '"' || t[t.length() - 1] == '\'') t.pop_back();
	return t;
}

#define fclose_m(file_ptr) do { fclose(file_ptr); file_ptr = NULL; } while (false)

// Uses for debug:
// A support of none-stream

#if MINSERVER_DEBUG == 1 || MINSERVER_DEBUG == 3
#define cout_d cout
#define endl_d endl
#define MINSERVER_EXT_DEBUG 1
int no_data_screen = 1;
#define heap_test() do {char *c = new char[2]; delete[] c; } while (false)
#else
#if MINSERVER_DEBUG == 2 || MINSERVER_DEBUG == 3 || MINSERVER_DEBUG == 4
// To make sure there is no memory leak or cause c0000374
#define heap_test() do {char *c = new char[2]; delete[] c; } while (false)
#else
#define heap_test() __noop
#endif
class null_stream {
public:
	null_stream() {

	}
} n_stream;

// For some reason ??
template <typename Ty>
null_stream& operator << (null_stream &origin, Ty other) {
	return origin;
}
#define cout_d n_stream
#define endl_d nullptr
int no_data_screen = 0;
#endif

/*
#define delete __delete_helper <<

#pragma push_macro("delete")
#undef delete

class __delete {
	__delete() {

	}
} __delete_helper;

void operator << (__delete dobj, void *block) {
	delete block;
}

#pragma pop_macro("delete")
*/