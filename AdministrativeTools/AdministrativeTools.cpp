// AdministrativeTools.cpp : 定义 DLL 的导出函数。
//

#include "AdministrativeTools.h"

int memode = 0;
char *filepath;

mem_alloc MemoryAllocate;

extern "C" bool StringEqual(cc_str a, cc_str b) {
	return strcmp(a, b) == 0;
}

extern "C" c_str CopyStr(cc_str target, int len = -1) {
	if (len < 0) len = strlen(target);
	c_str res = (char*)MemoryAllocate(len+1);
	strcpy(res, target);
	return res;
}

extern "C" void InResolve(cc_str data, int nextrec) {
	switch (nextrec) {
	case 1:
		// Method
		if (StringEqual(data, "publish")) memode = 1;
		break;
	case 2:
		// 'file'
		filepath = CopyStr(data);
		break;
	}
}

extern "C" ADMINISTRATIVETOOLS_API send_info ServerMain(cc_str data, sdata *s, void *out) {
	MemoryAllocate = s->mc_lib.m_alloc;
	/*
	Supported method=
	1. publish,	file=...
	*/
	////////////////////////////////////////
	// Sendup constant
	static const char sendup[] = { "HTTP/1.1 %d %s\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	static const char badoper[] = { "Operation not exist" };
#pragma region(Resolver)
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
	// End of resolving
#pragma endregion

	send_info se;
	FILE *f = nullptr;

	switch (memode) {
	case 1:
		f = fopen(s->cal_lib.public_file_path, "a");
		fprintf(f, "%s\n", filepath);
		fclose(f);
		break;
	default:
		// Send 'nothing' message.
		char *tmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(sendup) + strlen(badoper) + 20));
		sprintf(tmp, sendup, 200, "OK", strlen(badoper), badoper);
		se.cdata = CopyStr(tmp);
		se.len = strlen(tmp);
		s->mc_lib.m_free(tmp);
	}

	return se;
}