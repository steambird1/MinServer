#pragma once

// This is DLL (C) Framework for MinServer external DLL.

#ifdef __cplusplus
extern "C" {
#endif

#if ! __BOOL_DEFINED
#define bool int
#endif

	typedef int(*uidreq_request_func)(int);
	typedef bool(*uidreq_vaild_func)(int);
	typedef void(*uidreq_release_func)(int);

	typedef struct _callers {
		uidreq_request_func uidc_request;
		uidreq_vaild_func uidc_vaild;
		uidreq_request_func uidc_uidof;
		uidreq_release_func uidc_rel;
	} callers;

	typedef struct _sdata {
		callers cal_lib;
	} sdata;

#ifdef __cplusplus
}
#endif 