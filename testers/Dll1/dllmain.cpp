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
		char *stmp = new char[strlen(sendup) + 100], t[150];
		sprintf(t, "MinServer Diag\nTester DLL.dll\n\nValues:\nCurrent Memory Usage State: %.2lf", sdata->cal_lib.mem_usage());
		sprintf(stmp, sendup, strlen(t), t);
		/*sprintf(stmp, sendup, strlen(cb), cb);
		printf("DLL: Outputting returning: \n%s\n", stmp);*/
		return { (int)strlen(stmp), stmp };
		//return sendup;
	}
#ifdef __cplusplus
}
#endif