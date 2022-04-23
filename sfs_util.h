#pragma once
#include <Windows.h>
#include <string>
#include <cstdio>
#include "framework.h"
using namespace std;


BOOL FindFirstFileExists(LPCSTR lpPath, DWORD dwFilter)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFileA(lpPath, &fd);
	BOOL bFilter = (FALSE == dwFilter) ? TRUE : fd.dwFileAttributes & dwFilter;
	BOOL RetValue = ((hFind != INVALID_HANDLE_VALUE) && bFilter) ? TRUE : FALSE;
	FindClose(hFind);
	return RetValue;
}

// Is file exists?
BOOL FilePathExists(LPCSTR lpPath)
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
	if (cws == 0) return bytes();
	FILE *rs = fopen(cwtemp.c_str(), "rb"); // not 'r'
	//auto rs = file_object(cwtemp, "rb");
	fseek(rs, 0, SEEK_SET); // to head
	char *sending = new char[cws + 10];
	memset(sending, 0, sizeof(sending));
	fread(sending, sizeof(char), cws, rs);
	sending[cws] = '\0';
	fclose(rs);
	bytes b;
	b.add(sending, cws);
	return move(b);
}

string sToLower(string s) {
	string t = "";
	for (auto &i : s) t += tolower(i);
	return t;
}

bool isBeginWith(string a, string b) {
	if (a.length() <= b.length()) {
		return (b.substr(0, a.length()) == a);
	}
	return false;
}