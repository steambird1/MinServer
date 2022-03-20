#pragma once

// This is DLL (C compile rules) Framework for MinServer external DLL.
// Notice: Don't forgot to FREE MEMORY SPACE using given library.

extern "C" {

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdio>
#include <cstring>
#include <windows.h>
#include <yvals.h>
	using namespace std;

#if ! __BOOL_DEFINED
	typedef int bool;
#define true 1
#define false 0
#endif

	// Declaration
	typedef struct _sdata sdata;

	// This components of code requires sync with util.h/_CONN_DEFINED.
#ifndef _CONN_DEFINED
#define _CONN_DEFINED
	typedef char *c_str;
	typedef const char *cc_str;

	struct _single_ip_info {
		cc_str ip_addr;
		int ip_vis;
	};

	typedef struct _ip_info {
		int len;
		struct _single_ip_info *data;
	} ip_info;

	typedef int(*uidreq_request_func)(int);
	typedef bool(*uidreq_vaild_func)(int);
	typedef void(*uidreq_release_func)(int);
	typedef bool(*uoperator_auth_func)(int, cc_str);
	typedef bool(*foperator_release_func)(int);
	typedef int(*foperator_open_func)(int, cc_str, cc_str);
	// New updates:
	typedef double(*health_func)(void);
	typedef ip_info(*ip_health_func)(void);
	typedef void(*ugroup_insert_func)(int, int);	// uid -- group
	typedef bool(*ugroup_query_func)(int, int);
	typedef void(*uoperator_mod_func)(int, cc_str);
	typedef void(*uoperator_chg_perm)(cc_str, int, int);
	typedef bool(*uoperator_exists_func)(int);			// Is specified UID exists?

	typedef struct _callers {
		uidreq_request_func uidc_request;
		uidreq_vaild_func uidc_vaild;
		uidreq_request_func uidc_uidof;
		uidreq_release_func uidc_rel;
		uoperator_auth_func	uop_auth;
		foperator_release_func fop_rel;
		foperator_open_func fop_open;
		health_func mem_usage;			// Unit: MB
		health_func utoken_usage;		// 0.0 - 1.0
		health_func ftoken_usage;		// 0.0 - 1.0
		ip_health_func iphealth_info;
		ugroup_insert_func ug_insert;
		ugroup_insert_func ug_remove;
		ugroup_query_func ug_query;		// Is it in specified group?
		uoperator_mod_func uop_mod;		// Automaticly decides
		uoperator_chg_perm uop_chp;		// Automaticly changing to '-1' means changing owner
		uoperator_exists_func uop_exists;
		cc_str e403, e404, e501, e200_ok, e200_redirect;	// IDs
		// New updates:
		cc_str perm_data_path;// = "$permission.txt";
		cc_str user_data_path;// = "$users.txt";
		cc_str public_file_path;// = "$public.txt";
		cc_str group_path;// = "$groups.txt";
		cc_str assiocate_path;// = "$assiocate.txt";
		cc_str redirect_path;// = "$redirect.txt";
		cc_str dll_path;// = "$dlls.txt";
		cc_str ban_path;// = "$bans.txt";
	} callers;
	
	typedef void*(*mem_alloc)(size_t);
	typedef void(*mem_free)(void*);
	
	typedef struct _mem_callers {
		mem_alloc m_alloc;
		mem_free m_free;
	} mem_callers;

	typedef struct _sdata {
		callers cal_lib;
		mem_callers mc_lib;
		int *m_error;
	} sdata, asdata;

	typedef struct _send_info {
		int len;
		cc_str cdata;
	} send_info;
	// Return value now not used
	typedef send_info(*d_func)(cc_str, sdata*, void*);
	typedef send_info(*as_func)(cc_str, cc_str, asdata*, void*);	// Assiocated caller. asdata* currently NULL. another one is file content
#endif

	// By this way, Intellisense can read them and we can edit them
#if !defined(_CP_DEFINED) || defined(__INTELLISENSE__)
#define _CP_DEFINED

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

	struct _single_cpost_info {
		struct {
			int len;
			c_pair *param;
		} attr;
		int cs_len;
		c_str content;
	};

	typedef struct _cpost_info {
		struct {
			int len;
			struct _single_cpost_info *param;
		} data;
	} cpost_info;

#define callocer(count, objsize) (mallocer(count*objsize))

	recv_info c_resolve(const char *req, mem_alloc mallocer) {
		char buf[64];
		recv_info res = {};
		res.method = (char*)callocer(8, sizeof(char));
		res.path = (char*)callocer(MAX_PATH, sizeof(char));
		res.proto = (char*)callocer(16, sizeof(char));
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
		res.attr.param = (c_pair*)callocer(sr_len + 1, sizeof(c_pair));
		for (int i = 0; i < sr_len; i++) {
			res.attr.param[i].key = (char*)callocer(128, sizeof(char));
			res.attr.param[i].value = (char*)callocer(512, sizeof(char));
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
		res.content = (char*)callocer(cont_len + 1, sizeof(char));
		for (int i = 0; i < cont_len; i++) res.content[i] = req[r_ptr + i];
		return res;
	}

	const char* c_boundary(const char *ctypes, mem_alloc mallocer) {
		int sl = strlen(ctypes);
		int bs = 0, pt = 0;
		char *tmp = (char*)callocer(60, sizeof(char));	// Boundary in usually 60
		for (int i = 0; i < sl; i++) {
			if (bs == 2) {
				tmp[pt++] = ctypes[i];
			}
			else if (ctypes[i] == ';' && bs == 0) bs = 1;
			else if (ctypes[i] == '=' && bs == 1) bs = 2;
		}
		if (tmp[sl - 1] == '\r') tmp[sl - 1] = '\0';
		//tmp[sl+1] = '\0';
		return tmp;
	}

	bool c_bstrcmp(const char *wl, const char *bound) {
		char *wp = new char[90], *bounp = new char[90], *wt, *bount;
		strcpy(wp, wl);
		strcpy(bounp, bound);
		wt = wp; bount = bounp;
		while (*wt == '-') wt++;
		while (*bount == '-') bount++;
		int res = (strcmp(wt, bount));
		delete[] wp, bounp;
		return res;	// res == 0 -> OK
	}

	// Gets how many succeed (Automaticly stopped if size > read_buffer or count > read_count..)
	cpost_info c_postres(const char *content, const char *boundary, int content_length, int read_count, int read_buffer, mem_alloc mallocer) {
		static const int para_count = 16;
		// End

		cpost_info res;
		//for (int i = 0; i < read_count; i++) {
		res.data.len = 0;
		res.data.param = (struct _single_cpost_info*)callocer(read_count, sizeof(struct _single_cpost_info));
		for (int i = 0; i < read_count; i++) {
			res.data.param[i].attr.param = (c_pair*)callocer(para_count, sizeof(c_pair));
			for (int j = 0; j < para_count; j++) {
				res.data.param[i].attr.param[j].key = (char*)callocer(64, sizeof(char));
				res.data.param[i].attr.param[j].value = (char*)callocer(128, sizeof(char));
			}
			res.data.param[i].content = (char*)callocer(read_buffer, sizeof(char));
		}
		//}
		char ldata[1024] = {};
		int lptr = 0, cptr = -1, cparam = 0, clptr = 0;	// Start in 0
		int state = -1, astate = false, errored = false;
		for (int i = 0; i < content_length; i++) {
			if (content[i] == '\n') {
				if (lptr == 0 || (lptr == 1 && ldata[0] == '\r')) {
					// Empty line, data start
					clptr = 0;
					state = false;
				}
				else if (lptr < 90 && (c_bstrcmp(ldata, boundary) == 0)) {
					// Another boundary start
					if (!errored) {
						cptr++;
						res.data.len++;
					}
					if (cptr >= read_count)
						break;

					errored = false;
					state = true;
				}
				else {
					char lp = ldata[lptr - 2];
					ldata[lptr - 2] = '\0';
					if (lptr < 90 && (c_bstrcmp(ldata, boundary) == 0)) {
						break;
					}
					else {
						ldata[lptr - 2] = lp;
					}
					if (state) {
						cparam++;
						res.data.param[cptr].attr.len++;
						clptr = 0;
						astate = false;
					}
					else if (state == 0) {
						// Data
						if (clptr >= read_buffer) {
							errored = true;
							continue;
						}
						res.data.param[cptr].cs_len = lptr;
						for (int i = 0; i <= lptr; i++)
							res.data.param[cptr].content[clptr++] = ldata[i];

					}
				}

				memset(ldata, 0, sizeof(ldata));
				lptr = 0;
			}
			else {
				ldata[lptr++] = content[i];
				if (state == 1) {
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
			}
			
		}
		return res;

	}

	typedef struct _send_para {
		c_pair *cp;
		int cp_len;				// Attributes
		c_str proto, stde;		// Proto var and response type
		int recode;				// Response code
		c_str content;
		int clen;				// Content and its length
	} send_para;

	send_info c_send(send_para sp, int alloc_len, mem_alloc mallocer) {
		int pre_len = 4;	// like 2 EOL
		char header[100];
		sprintf(header, "%s %d %s", sp.proto, sp.recode, sp.stde);
		pre_len += strlen(header);
		for (int i = 0; i < sp.cp_len; i++) {
			pre_len += (strlen(sp.cp[i].key) + strlen(sp.cp[i].value) + strlen(": \n"));
		}
		pre_len += sp.clen;
		char *res = (char*)mallocer(pre_len+20);	// Prevent too long prelen
		strcpy(res, header);
		for (int i = 0; i < sp.cp_len; i++) {
			sprintf(res, "%s\n%s: %s", res, sp.cp[i].key, sp.cp[i].value);
		}
		sprintf(res, "%s\n\n", res);
		size_t res_p = sizeof(char) * strlen(res);
		memcpy(res + res_p, sp.content, sizeof(char) * sp.clen);
		return { pre_len, res };
	}

#endif
	
}
