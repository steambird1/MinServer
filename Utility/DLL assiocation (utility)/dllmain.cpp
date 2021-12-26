// An example of assiocation (assiocate as DLL auto-open).
#include "../../c_framework.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

extern "C" __declspec(dllexport) send_info AssiocateMain(cc_str receive, cc_str file_path, asdata *as) {
	HINSTANCE h = LoadLibraryA(file_path);
	d_func df = (d_func)GetProcAddress(h, "ServerMain");
	const char error[] = { "Requested DLL can't be runned" };
	static const char *sendup = { "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	char tmp[1000];
	if (df == NULL) {
		sprintf(tmp, sendup, strlen(error), error);
		return { (int)strlen(tmp), tmp };
		//return { (int)strlen(error), error };
	}
	else {
		return df(receive, as);
	}
}
