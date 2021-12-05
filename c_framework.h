#pragma once
#include <cstdio>
#include <cstring>
#include <windows.h>
using namespace std;

// This is DLL (C) Framework for MinServer external DLL.
// Notice: Don't forgot to FREE MEMORY SPACE.

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

	struct _single_post_info {
		struct {
			int len;
			c_pair *param;
		} attr;
		c_str content;
	};

	typedef struct _post_info {
		struct {
			int len;
			struct _single_post_info *param;
		} data;
	} post_info;

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
		int mode_len = 0, cur_len = 0, cont_len = 0;
		while ((!(req[r_ptr] == '\n' && (req[r_ptr - 1] == '\n' || req[r_ptr - 2] == '\n'))) && r_ptr < r_len) {
			if (req[r_ptr] == '\n') {
				if (strcmp(res.attr.param[cur_len].key, "Content-Length") == 0) {
					cont_len = atoi(res.attr.param[cur_len].value);
				}
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
		res.content = (char*)calloc(cont_len + 1, sizeof(char));
		for (int i = 0; i < cont_len; i++) res.content[i] = req[r_ptr + i];
		return res;
	}
	
	// Gets how many succeed (Automaticly stopped if size > read_buffer or count > read_count..)
	post_info c_postres(const char *content, const char *boundary, int content_length, int read_count, int read_buffer) {
		post_info res;
		//for (int i = 0; i < read_count; i++) {
		res.data.param = (struct _single_post_info*)calloc(read_count, sizeof(struct _single_post_info));
		for (int i = 0; i < read_count; i++) {
			res.data.param[i].attr.param = (c_pair*)calloc(16, sizeof(c_pair));
			for (int j = 0; j < 16; j++) {
				res.data.param[i].attr.param[i].key = (char*)calloc(64, sizeof(char));
				res.data.param[i].attr.param[i].value = (char*)calloc(128, sizeof(char));
			}
			res.data.param[i].content = (char*)calloc(read_buffer, sizeof(char));
		}
		//}
		char ldata[1024] = {};
		int lptr = 0, cptr = 0, cparam = 0, clptr = 0;
		bool state = false, astate = false;
		for (int i = 0; i < content_length; i++) {
			if (content[i] == '\n') {
				clptr = 0;
				if (lptr == 0) {
					// Empty line, data start
					state = false;
				}
				else if (strcmp(ldata, boundary) == 0) {
					// Another boundary start
					cptr++;
					state = true;
					memset(ldata, 0, sizeof(ldata));
				}
				else {
					ldata[lptr - 2] = '\0';
					if (strcmp(ldata, boundary) == 0) {
						break;
					}
					if (state) {
						astate = false;
					}
				}
			} else {
				if (state) {
					// Args
					if (content[i] == ':') {
						i++;	// ' '
						clptr = 0;
						astate = true;
					}
					else if (astate) {
						res.data.param[cptr].attr.param[cparam].value[clptr++] = content[i];
					}
					else {
						res.data.param[cptr].attr.param[cparam].key[clptr++] = content[i];
					}
				}
				else {
					// Data
					res.data.param[cptr].content[clptr++] = content[i];
				}
			}
		}
		return res;
	}

#ifdef __cplusplus
}
#endif 