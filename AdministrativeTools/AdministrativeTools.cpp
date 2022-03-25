// AdministrativeTools.cpp : 定义 DLL 的导出函数。
//

#include "AdministrativeTools.h"

int memode = 0;
char filepath[MAX_PATH];

extern "C" void InResolve(cc_str data, int nextrec) {
	switch (nextrec) {
	case 1:
		// Method
		break;
	case 2:
		// 'file'
		break;
	}
}

ADMINISTRATIVETOOLS_API extern "C" send_info ServerMain(cc_str data, sdata *s, void *out) {
	/*
	Supported method=
	1. publish,	file=...
	*/
	// Resolver from filesearcher
	char path[MAX_PATH * 3];
	int res = 0, pptr = 0; // Path in the middle.
	for (const char *i = data; (*i) != '\n' && res <= 1; i++) {
		char ir = (*i);
		if (ir == ' ') res++;
		else if (res == 1) path[pptr++] = ir;
	}
	path[pptr] = '\0';
	////////////////////////////////////////
	// Sendup constant
	static const char sendup[] = { "HTTP/1.1 %d %s\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	////////////////////////////////////////
	// Get keys first
	// Get parameters
	bool flag = false;
	int nextrec = 0, bp = 0;
	char buf2[100];
#define bpclr() do { memset(buf2, 0, sizeof(buf2)); bp = 0; } while (false)
	for (size_t i = 0; i < strlen(path); i++) {
		if (flag) {
			if (path[i] == '=') {
				if (strcmp(buf2, "method") == 0) nextrec = 1;
				else if (strcmp(buf2, "file") == 0) nextrec = 2;
				else nextrec = 0;
				bpclr();
			}
			else if (path[i] == '&') {
				// switch nextrec to do...
				InResolve(buf2, nextrec);
			}
			else {
				buf2[bp++] = path[i];
			}

		}
		if (path[i] == '?') flag = true;
	}
	if (nextrec) {
		// same switch ...
		InResolve(buf2, nextrec);
	}
#ifdef bpclr
#undef bpclr
#endif
}