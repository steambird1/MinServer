#pragma once

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

#ifdef __cplusplus
}
#endif 