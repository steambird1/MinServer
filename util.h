#pragma once
#define CRTDBG_MAP_ALLOC
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <string>
#include <crtdbg.h>
//#include "c_framework.h"
using namespace std;

BOOL FindFirstFileExists(LPCTSTR lpPath, DWORD dwFilter)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(lpPath, &fd);
	BOOL bFilter = (FALSE == dwFilter) ? TRUE : fd.dwFileAttributes & dwFilter;
	BOOL RetValue = ((hFind != INVALID_HANDLE_VALUE) && bFilter) ? TRUE : FALSE;
	FindClose(hFind);
	return RetValue;
}

// Is file exists?
BOOL FilePathExists(LPCTSTR lpPath)
{
	return FindFirstFileExists(lpPath, FALSE) && (!FindFirstFileExists(lpPath, FILE_ATTRIBUTE_DIRECTORY));
}

bool fileExists(string path) {
	return FilePathExists(path.c_str());
}

// Already modified...
int getSize(string filename) {
	FILE *f = fopen(filename.c_str(), "r");
	if (f == NULL) return 0;
	fseek(f, 0, SEEK_END);
	int res = ftell(f);
/*	int res = 0;
	while (!feof(f)) {
		fgetc(f);
		res++;
	}*/
	fclose(f);
	return res;
}

bytes readAll(string cwtemp) {
	int cws = getSize(cwtemp);
	FILE *rs = fopen(cwtemp.c_str(), "rb"); // not 'r'
	fseek(rs, 0, SEEK_SET); // to head
	char *sending = new char[cws + 10];
	memset(sending, 0, sizeof(sending));
	fread(sending, sizeof(char), cws, rs);
	sending[cws] = '\0';
	fclose(rs);
	bytes b;
	b.add(sending, cws);
	return b;
}

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

string makeTemp(void) {
	string s;
	do {
		s = string(getenv("temp")) + "\\" + to_string(random());
	} while (fileExists(s));
	return s;
}

const bytes not_found_d = "<html><head><title>Page not found - 404</title></head><body><h1>404 Not found</h1><p>Requested page not found on this server.</p><hr /><p>MinServer 2.0</p></body></html>";
const bytes not_supported_d = "<html><head><title>Not Implemented - 501</title></head><body><h1>Not Implemented</h1><p>Request not implemented by server.</p><hr /><p>MinServer 2.0</p></body></html>";
const bytes no_perm_d = "<html><head><title>Forbidden - 403</title></head><body><h1>Forbidden</h1><p>You can't view this.</p><hr /><p>MinServer 2.0</p></body></html>";

bytes not_found_c = not_found_d, not_supported_c = not_supported_d, no_perm_c = no_perm_d;

// !! It requires sprintf() with redirection information !!
string ok = "<html><head><title>OK - 200</title></head><body><h1>OK</h1><p>Requested operation completed successfully.</p>%s<hr /><p>MinServer 2.0</p></body></html>";
// !! It requires sprintf() for the link and ENCODED LINK and sprintf to 'ok' !!
string redirect = "<p>You are going to be redirect. If this page does not redirect automaticly, click <a href=\"%s\">here</a>.</p><script>window.location.href = \"%s\"</script>";

// Controller class.
class uidctrl {
public:
	static int request(int uid) {
		if (uid_to_token.count(uid)) {
			token_to_uid.erase(uid_to_token[uid]);
		}
		int tk;
		do {
			tk = random();
		} while (token_to_uid.count(tk));
		token_to_uid[tk] = uid;
		uid_to_token[uid] = tk;
		return tk;
	}
	static bool vaild(int token) {
		if (!token_to_uid.count(token)) return false;
		if (!uid_to_token.count(token_to_uid[token])) return false;
		return true;
	}
	static int uidof(int token) {
		return token_to_uid[token];
	}
	static void release(int token) {
		uid_to_token.erase(token_to_uid[token]);
		token_to_uid.erase(token);
	}
	static int size() {
		return token_to_uid.size();
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

string sToLower(string s) {
	string t = "";
	for (auto &i : s) t += tolower(i);
	return t;
}

string sRemovingEOL(string s) {
	string t = s;
	while (t[t.length() - 1] == '\n') t.pop_back();
	return t;
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

#define MINSERVER_DEBUG 5
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
#define vs_heap_test() __noop
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

#define CONN_DEFINED
typedef char *c_str;
typedef const char *cc_str;

typedef int(*uidreq_request_func)(int);
typedef bool(*uidreq_vaild_func)(int);
typedef void(*uidreq_release_func)(int);
typedef bool(*uoperator_auth_func)(int, cc_str);
typedef bool(*foperator_release_func)(int);
typedef int(*foperator_open_func)(int, cc_str, cc_str);
// New updates:

typedef struct _single_ip_info {
	cc_str ip_addr;
	int ip_vis;
} single_ip_info;

typedef struct _ip_info {
	int len;
	single_ip_info *data;
} ip_info;

typedef double(*health_func)(void);
typedef ip_info(*ip_health_func)(void);
typedef void(*ugroup_insert_func)(int, int);	// uid -- group
typedef bool(*ugroup_query_func)(int, int);
typedef void(*uoperator_mod_func)(int, cc_str);
typedef void(*uoperator_chg_perm)(cc_str, int, int);
typedef bool(*uoperator_exists_func)(int);			// Is specified UID exists?

typedef struct _callers {
	uidreq_request_func uidc_request;
	uidreq_vaild_func uidc_vaild;
	uidreq_request_func uidc_uidof;
	uidreq_release_func uidc_rel;
	uoperator_auth_func	uop_auth;
	foperator_release_func fop_rel;
	foperator_open_func fop_open;
	health_func mem_usage;			// Unit: MB
	health_func utoken_usage;		// 0.0 - 1.0
	health_func ftoken_usage;		// 0.0 - 1.0
	ip_health_func iphealth_info;
	ugroup_insert_func ug_insert;
	ugroup_insert_func ug_remove;
	ugroup_query_func ug_query;		// Is it in specified group?
	uoperator_mod_func uop_mod;		// Automaticly decides
	uoperator_chg_perm uop_chp;		// Automaticly changing to '-1' means changing owner
	uoperator_exists_func uop_exists;
	// New updates:
	cc_str e403, e404, e501;	// IDs
	struct {
		cc_str ok, redirect;
	} e200;
} callers;

typedef struct _sdata {
	callers cal_lib;
} sdata, asdata;

typedef struct _send_info {
	struct {
		int len;
		cc_str cdata;
	};
} send_info;
typedef send_info(*d_func)(cc_str, sdata*);
typedef send_info(*as_func)(cc_str, cc_str, asdata*);	// Assiocated caller. asdata* currently NULL.