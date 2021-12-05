// This is a test DLL file for DLL function. Do not try running this.

#define _CRT_SECURE_NO_WARNINGS
#include "../../c_framework.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) send_info ServerMain(const char *data, sdata *sdata) {
		static const char *sendup = { "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
		char *stmp = new char[strlen(sendup) + 100];
		recv_info r = c_resolve(data);
		const char *cb = new char[90];
		int cl = 0;
		for (int i = 0; i < r.attr.len; i++) {
			if (strcmp(r.attr.param[i].key, "Content-Length") == 0) {
				cl = atoi(r.attr.param[i].value);
				//break;
			}
			else if (strcmp(r.attr.param[i].key, "Content-Type") == 0) {
				cb = c_boundary(r.attr.param[i].value);
				//strcpy(cb, cbc);
			}
		}
		
		cpost_info c = c_postres(r.content, cb, cl, 4, 2048);
		printf("DLL Output: %d\n", c.data.len);
		if (c.data.len >= 2) {
			sprintf(stmp, sendup, strlen(c.data.param[1].content), c.data.param[1].content);
		}
		else {
			sprintf(stmp, sendup, 4, "NONE");
		}
		/*sprintf(stmp, sendup, strlen(cb), cb);
		printf("DLL: Outputting returning: \n%s\n", stmp);*/
		return { (int)strlen(stmp), stmp };
		//return sendup;
	}
#ifdef __cplusplus
}
#endif