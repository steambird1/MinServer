// This is a test DLL file for DLL function. Do not try running this (behavior is undefined).

#define _CRT_SECURE_NO_WARNINGS
#include "../../c_framework.h"
#include <cstdio>
#include <cstring>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) send_info ServerMain(const char *data, sdata *sdata) {
		// 301 Moved Permanently
		static const char *sendup = { "HTTP/1.1 301 Moved Permanently\nContent-Length: %d\n\n%s" };
		char t[30];
		char *stmp;
		sprintf(t, "/cc.html");
		stmp = (char*)sdata->mc_lib.m_alloc(sizeof(char) *(strlen(t) + strlen(sendup) + 20));
		/*size_t tt = strlen(t);
		int i;
		const int llen = 2000;
		for (i = 0; i < llen; i++) t[tt + i] = 'A';
		t[tt + llen] = '\0';*/
		sprintf(stmp, sendup, strlen(t), t);
		/*sprintf(stmp, sendup, strlen(cb), cb);
		printf("DLL: Outputting returning: \n%s\n", stmp);*/
		return { (int)strlen(stmp), stmp };
		//return sendup;
	}
#ifdef __cplusplus
}
#endif