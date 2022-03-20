#include "ApplicationLauncher.h"

extern "C" APPLICATIONLAUNCHER_API send_info AssiocateMain(cc_str received, cc_str path, asdata *as, void *out) {
	static const char sendup[] = { "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	char tmp[300], res[32], fdest[100], cdest[200];
	// Writes down.
	srand(NULL);
	itoa(rand(), res, 10);
	sprintf(fdest, "%s\\%s", getenv("temp"), res);
	FILE *fd = fopen(fdest, "wb");
	// Resolve the size.
	recv_info rc = c_resolve(received, as->mc_lib.m_alloc);
	// Find content length
	int cl = 0;
	for (size_t i = 0; i < rc.attr.len; i++) {
		if (strcmp(rc.attr.param[i].key, "Content-Length") == 0) {
			cl = atoi(rc.attr.param[i].value);
			break;
		}
	}
	fwrite(rc.content, sizeof(char), cl, fd);
	fclose(fd);
	fd = NULL;
	// Just returns execution code.
	// Add filename as arg.
	sprintf(cdest, "%s %s", path, fdest);
	int ru = system(cdest);
	itoa(ru, res, 10);
	sprintf(tmp, sendup, res);
	send_info sd;
	sd.cdata = (char*)mmalloc(strlen(tmp));
	sd.len = strlen(tmp);
	return sd;
	//(*out) = sd;

	//return {};
}