#define _CRT_SECURE_NO_WARNINGS

#include "FileSearcher.h"
#include <cstring>
#include "../c_framework.h"
using namespace std;

/*
Specify:
1. utoken= (Reserved)
2. fname=
*/


extern "C" FILESEARCHER_API send_info ServerMain(const char *data, sdata *sdata, void *out) {
//	recv_info rc = c_resolve(data, sdata->mc_lib.m_alloc);
	// What will happen if we not use c_resolve?
	// Get raw path just from data, not caring about data remaining
	char path[MAX_PATH * 3];
	int res = 0, pptr = 0; // Path in the middle.
	for (const char *i = data; (*i) != '\n' && res <= 1; i++) {
		char ir = (*i);
		if (ir == ' ') res++;
		else if (res == 1) path[pptr++] = ir;
	}
	path[pptr] = '\0';
	static const char sendup[] = { "HTTP/1.1 %d %s\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	// Get keys first
	int utoken = 0;
	bool usesub = false;
	char *subname = nullptr;
	// Get parameters
	bool flag = false;
	int nextrec = 0, bp = 0;
	char buf2[100];
#define bpclr() do { memset(buf2, 0, sizeof(buf2)); bp = 0; } while (false)
	for (size_t i = 0; i < strlen(path); i++) {
		if (flag) {
			if (path[i] == '=') {
				if (strcmp(buf2, "utoken") == 0) nextrec = 1;
				else if (strcmp(buf2, "fname") == 0) nextrec = 2;
				else nextrec = 0;
				bpclr();
			}
			else if (path[i] == '&') {
				switch (nextrec) {
				case 1:
					utoken = atoi(buf2);
					break;
				case 2:
					usesub = true;
					subname = (char*)sdata->mc_lib.m_alloc(bp);
					strcpy(subname, buf2);
					break;
				}
				nextrec = 0;
				bpclr();
			}
			else {
				buf2[bp++] = path[i];
			}

		}
		if (path[i] == '?') flag = true;
	}
	if (nextrec) {
		switch (nextrec) {
		case 1:
			utoken = atoi(buf2);
			break;
		case 2:
			usesub = true;
			subname = (char*)sdata->mc_lib.m_alloc(bp);
			strcpy(subname, buf2);
			break;
		}
	}
#ifdef bpclr
#undef bpclr
#endif
	// Read for all
	char *rtmp[65535], *tmp, *wtmp;
	int pt = 0, le = 0;
	char buf[MAX_PATH];
	FILE *rd = fopen(sdata->cal_lib.perm_data_path, "r");
	if (rd != NULL) {
		while (!feof(rd)) {
			fscanf(rd, "%*d%s%*d", buf);	// Where 'buf' is the file name
			int tt = strlen(buf);
			le += (tt+1);
			rtmp[pt] = (char*)sdata->mc_lib.m_alloc(tt + 1);
			strcpy(rtmp[pt], buf);
			pt++;		// Last PT should be ignored
		}
		fclose(rd);
		wtmp = (char*)sdata->mc_lib.m_alloc(le + pt + 40);
		memset(wtmp, 0, sizeof(wtmp));
		for (size_t i = 0; i < pt; i++) {
			if (usesub) {
				if (strstr(rtmp[i], subname) == NULL) continue;
			}
			sprintf(wtmp, "%s\n%s", wtmp, rtmp[i]);
//			sdata->mc_lib.m_free(rtmp[i]);
		}
		tmp = (char*)sdata->mc_lib.m_alloc(strlen(wtmp) + strlen(sendup) + 40);
		sprintf(tmp, sendup, 200, "OK", strlen(wtmp), wtmp);
	}
	else {
		tmp = (char*)sdata->mc_lib.m_alloc(strlen(sendup)+40);
		sprintf(tmp, sendup, 500, "Internal server error", 0, "");
	}
	send_info s;
	s.cdata = tmp;
	s.len = strlen(tmp);
	return s;
	//(*out) = s;//??

	//return {};
}