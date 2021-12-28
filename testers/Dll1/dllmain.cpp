// This is a test DLL file for DLL function. Do not try running this.

#define _CRT_SECURE_NO_WARNINGS
#include "../../c_framework.h"
#include <cstdio>
#include <cstring>
using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) send_info ServerMain(const char *data, sdata *sdata) {
		static const char *sendup = { "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
		char stmp[3000], t[2000];
		sprintf(t, "MinServer Diag\nTester DLL.dll\n\nValues:\nCurrent Memory Usage State: %.2lf", sdata->cal_lib.mem_usage());
		size_t tt = strlen(t);
		int i;
		for (i = 0; i < 1000; i++) t[tt + i] = 'A';
		t[tt + 1000] = '\0';
		sprintf(stmp, sendup, strlen(t), t);
		/*sprintf(stmp, sendup, strlen(cb), cb);
		printf("DLL: Outputting returning: \n%s\n", stmp);*/
		return { (int)strlen(stmp), stmp };
		//return sendup;
	}
#ifdef __cplusplus
}
#endif