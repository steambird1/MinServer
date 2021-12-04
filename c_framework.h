#pragma once
#include <cstdio>
#include <cstring>
#include <windows.h>
using namespace std;

// This is DLL (C) Framework for MinServer external DLL.

#ifdef __cplusplus
extern "C" {
#endif

#if ! __BOOL_DEFINED
	typedef signed int bool;
#define true 1
#define false 0
#endif

	// Declaration
	typedef struct _sdata sdata;

	typedef char *c_str;
	typedef const char *cc_str;
	typedef cc_str(*d_func)(cc_str, sdata*);

	typedef int(*uidreq_request_func)(int);
	typedef bool(*uidreq_vaild_func)(int);
	typedef void(*uidreq_release_func)(int);
	typedef bool(*uoperator_auth_func)(int, cc_str);	// To be implemented - auth as password TEXT given in cc_str
	typedef bool(*foperator_release_func)(int);
	typedef int(*foperator_open_func)(int, cc_str, cc_str);

	typedef struct _callers {
		uidreq_request_func uidc_request;
		uidreq_vaild_func uidc_vaild;
		uidreq_request_func uidc_uidof;
		uidreq_release_func uidc_rel;
		uoperator_auth_func	uop_auth;
		foperator_release_func fop_rel;
		foperator_open_func fop_open;
	} callers;

	typedef struct _sdata {
		callers cal_lib;
	} sdata;

	typedef struct _c_pair {
		c_str key, value;
	} c_pair;

	typedef struct _recv_info {
		c_str proto, method, path;
		struct {
			int len;
			c_pair *param;
		} attr;
		c_str content;
	} recv_info;

	recv_info c_resolve(const char *req) {
		char buf[64];
		recv_info res = {};
		res.method = (char*)calloc(8, sizeof(char));
		res.path = (char*)calloc(MAX_PATH, sizeof(char));
		res.proto = (char*)calloc(16, sizeof(char));
		res.attr.len = 0;
		int ptr = 0, r_ptr = 0, sr_ptr, sr_len = 0, r_len = strlen(req);
		// Get first-line information
		while (req[r_ptr] != '\n' && r_ptr < r_len) {
			buf[ptr++] = req[r_ptr++];
		}
		r_ptr++;	// EOL remains
		sscanf(buf, "%s %s %s", res.method, res.path, res.proto);	// buf now occupied
		sr_ptr = r_ptr;
		while ((!(req[r_ptr] == '\n' && (req[r_ptr - 1] == '\n' || req[r_ptr - 2] == '\n'))) && r_ptr < r_len) {
			if (req[r_ptr] == '\n') sr_len++;
			r_ptr++;
		}
		sr_len++;
		r_ptr = sr_ptr;
		res.attr.len = sr_len;
		res.attr.param = (c_pair*)calloc(sr_len + 1, sizeof(c_pair));
		for (int i = 0; i < sr_len; i++) {
			res.attr.param[i].key = (char*)calloc(128, sizeof(char));
			res.attr.param[i].value = (char*)calloc(512, sizeof(char));
		}
		bool mode = false;
		int mode_len = 0, cur_len = 0;
		while ((!(req[r_ptr] == '\n' && (req[r_ptr - 1] == '\n' || req[r_ptr - 2] == '\n'))) && r_ptr < r_len) {
			if (req[r_ptr] == '\n') {
				mode = false;
				mode_len = 0;
				cur_len++;
			}
			else if (req[r_ptr] == ':') {
				mode = true;
				mode_len = 0;
				r_ptr++;		// External ' ' (space)
			}
			else {
				if (mode) {
					res.attr.param[cur_len].value[mode_len] = req[r_ptr];
				}
				else {
					res.attr.param[cur_len].key[mode_len] = req[r_ptr];
				}
				mode_len++;
			}
			r_ptr++;
		}
		r_ptr++;	// EOL Remains
		res.content = (char*)calloc(r_len - r_ptr + 1, sizeof(char));
		for (int i = 0; i < r_len - r_ptr; i++) res.content[i] = req[r_ptr + i];
		return res;
	}

#ifdef __cplusplus
}
#endif 