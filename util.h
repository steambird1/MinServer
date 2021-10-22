#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <string>
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

/*
https ://blog.csdn.net/goodnew/article/details/8446575
*/
int getSize(string filename) {
	FILE *f = fopen(filename.c_str(), "r");
	if (f == NULL) return 0;
	fseek(f, 0, SEEK_END);
	int res = ftell(f);
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
vector<string> defiles = { "", "\\index.html","\\index.htm" };
map<string, string> ctypes = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };


string defaultType = "text/plain";

string findType(string fn) {
	string ext = getExt(fn);
	if (!ctypes.count(ext))
		return defaultType;
	return ctypes[ext];
}

int random(void) {
	static int seed = time(NULL);
	srand(seed);
	seed = rand();
	return seed;
}

string makeTemp(void) {
	return string(getenv("temp")) + "\\" + to_string(random());
}

bytes not_found = "<html><head><title>Page not found - 404</title></head><body><h1>404 Not found</h1><p>Requested page not found on this server.</p><hr /><p>MinServer 1.0</p></body></html>";
bytes not_supported = "<html><head><title>Not Implemented - 501</title></head><body><h1>Not Implemented</h1><p>Request not implemented by server.</p><hr /><p>MinServer 1.0</p></body></html>";
