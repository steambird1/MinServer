#pragma once
#include "safe_memalloc.h"

extern "C" {
	void* malloc_ts(size_t sz) {
		return ts_malloc::alloc(sz);
	}
	void free_ts(void *ptr) {
		ts_malloc::free(ptr);
	}
}