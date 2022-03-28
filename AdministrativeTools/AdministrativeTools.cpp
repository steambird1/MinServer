// AdministrativeTools.cpp : 定义 DLL 的导出函数。
//

#include "AdministrativeTools.h"

static const char PasswordFilePath[] = { "$admtool_password.txt" };

// In '1' inside, not necessary to get password
static const char AlwaysAllowPath[] = { "$admtool_alwaysallow.txt" };

int memode = 0, uid = 0;
char *filepath, *value, mdbuf[128], pbuf[128];
bool auth_success = false;

mem_alloc MemoryAllocate;
md5_caller MD5Calcutor;

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
	FILE *f = nullptr;
	switch (nextrec) {
	case 1:
		// Method
		if (StringEqual(data, "publish")) memode = 1;
		else if (StringEqual(data, "setown")) memode = 2;
		else if (StringEqual(data, "md5")) memode = 3;
		break;
	case 2:
		// 'file'
		filepath = CopyStr(data);
		break;
	case 3:
		// 'password'
		// Make auth.
		f = fopen(PasswordFilePath, "r");
		char test[120];
		GetCurrentDirectoryA(120, test);
		if (f != NULL) {
			fgets(mdbuf, 128, f);
			if (mdbuf[strlen(mdbuf) - 1] == '\n') mdbuf[strlen(mdbuf) - 1] = '\0';
			strcpy(pbuf, MD5Calcutor(data));
			if (StringEqual(pbuf, mdbuf)) auth_success = true;
		}
		break;
	case 4:
		// 'uid'
		uid = atoi(data);
		break;
	case 5:
		// 'value'
		value = CopyStr(data);
	}
}

extern "C" ADMINISTRATIVETOOLS_API send_info ServerMain(cc_str data, sdata *s, void *out) {
	// DLL global variables won't refresh
	memode = 0;
	uid = 0;
	auth_success = false;
	FILE *af = fopen(AlwaysAllowPath, "r");
	if (af != NULL) {
		int opt;
		fscanf(af, "%d", &opt);
		if (opt & 1) auth_success = true;	// Not necessary to get that
		fclose(af);
	}
#pragma region(Function Share)
	MemoryAllocate = s->mc_lib.m_alloc;
	MD5Calcutor = s->ext_lib.md5;
#pragma endregion
	/*
	Supported method=
	1. publish,	file=...
	*/
#pragma region(Constants)
	static const char sendup[] = { "HTTP/1.1 %d %s\nContent-Type: text/plain\nContent-Length: %d\n\n%s" };
	static const char badoper[] = { "Operation not exist" };
	static const char success[] = { "Operation completed successfully" };
	static const char noauth[] = { "Please login first\nYour password MD5: %s" };
#pragma endregion
	
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
	bpclr();
	for (size_t i = 0; i < strlen(path); i++) {
		if (flag) {
			if (path[i] == '=') {
				if (strcmp(buf2, "method") == 0) nextrec = 1;
				else if (strcmp(buf2, "file") == 0) nextrec = 2;
				else if (strcmp(buf2, "password") == 0) nextrec = 3;
				else if (strcmp(buf2, "uid") == 0) nextrec = 4;
				else if (strcmp(buf2, "value") == 0) nextrec = 5;
				else nextrec = 0;
				bpclr();
			}
			else if (path[i] == '&') {
				// switch nextrec to do...
				InResolve(buf2, nextrec);
				bpclr();
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

	char *tmp = nullptr;
	send_info se;
	FILE *f = nullptr;

	if (!auth_success) {
		char *wtmp = nullptr;
		wtmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(noauth) + strlen(pbuf) + 20));
		sprintf(wtmp, noauth, pbuf);
		tmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(sendup) + strlen(wtmp) + 20));
		sprintf(tmp, sendup, 200, "OK", strlen(wtmp), wtmp);
		s->mc_lib.m_free(wtmp);
		goto dsend;
	}

	switch (memode) {
	case 1:
		f = fopen(s->cal_lib.public_file_path, "a");
		fprintf(f, "%s\n", filepath);
		fclose(f);
		goto success;
	case 2:
		s->cal_lib.uop_chp(filepath, uid, -1);
		goto success;
	case 3:
		tmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(sendup) + 80));
		strcpy(mdbuf, MD5Calcutor(value));
		sprintf(tmp, sendup, 200, "OK", strlen(mdbuf), mdbuf);
		break;
	default:
		// Send 'nothing' message.
		tmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(sendup) + strlen(badoper) + 50));
		sprintf(tmp, sendup, 500, "Not Implemented", strlen(badoper), badoper);
	}

	goto dsend;

	// Send 'success'
	success: tmp = (char*)s->mc_lib.m_alloc(sizeof(char)*(strlen(sendup) + strlen(success) + 20));
	sprintf(tmp, sendup, 200, "OK", strlen(success), success);

	// Default sender
	dsend: se.cdata = CopyStr(tmp);
	se.len = strlen(tmp);
	s->mc_lib.m_free(tmp);

	return se;
}