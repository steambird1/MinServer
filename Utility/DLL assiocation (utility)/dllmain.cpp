// An example of assiocation (assiocate as DLL auto-open).
#include "../../c_framework.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

extern "C" __declspec(dllexport) send_info AssiocateMain(cc_str receive, cc_str file_path, asdata *as) {
	HINSTANCE h = LoadLibraryA(file_path);
	d_func df = (d_func)GetProcAddress(h, "ServerMain");
	const char error[] = { "Requested DLL can't be runned" };
	if (df == NULL) {
		return { (int)strlen(error), error };
	}
	else {
		return df(receive, as);
	}
}
