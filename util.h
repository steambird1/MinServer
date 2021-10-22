#pragma once
#include <Windows.h>
#include <cstdio>
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

vector<string> defiles = { "index.html","index.htm" };
map<string, string> ctypes = { {".apk", "application/vnd.android"},  {".html","text/html"}, {".htm", "text/html"}, {".ico","image/ico"}, {".jpg", "image/jpg"}, {".jpeg", "image/jpeg"}, {".png", "image/apng"}, {".txt","text/plain"}, {".css", "text/css"}, {".js", "application/x-javascript"}, {".mp3", "audio/mpeg"}, {".wav", "audio/wav"}, {".mp4", "video/mpeg"} };

string defaultType = "text/plain";
