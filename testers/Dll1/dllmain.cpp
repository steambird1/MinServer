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
		//static const char *sendup = { "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
		char t[] = { "/cc.html" };
		// Let's make a test using csend
		send_para s = {};
		s.clen = strlen(t);
		//s.content = t;
		strcpy(s.content, t);	// As no special things
		s.cp = (c_pair*)sdata->mc_lib.m_alloc(sizeof(c_pair) * 3);
		s.clen = 2;
		for (int i = 0; i < s.clen; i++) {
			s.cp[i].key = (char*)sdata->mc_lib.m_alloc(sizeof(char) * 50);
			s.cp[i].value = (char*)sdata->mc_lib.m_alloc(sizeof(char) * 50);
		}
		sprintf(s.cp[0].key, "Content-Type");
		sprintf(s.cp[0].value, "text/plain");
		sprintf(s.cp[1].key, "Content-Length");
		sprintf(s.cp[1].value, "%d", s.clen);
		s.proto = (char*)sdata->mc_lib.m_alloc(sizeof(char) * 20);
		sprintf(s.proto, "HTTP/1.1");
		s.recode = 200;
		s.stde = (char*)sdata->mc_lib.m_alloc(sizeof(char) * 10);
		sprintf(s.stde, "OK");
		return c_send(s, 150, sdata->mc_lib.m_alloc);
	}
#ifdef __cplusplus
}
#endif