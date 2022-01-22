#pragma once
// Thread-safe memory allocate 'server'.
#include <thread>
#include <queue>
#include <map>
#include <cstdlib>
#include <ctime>
using namespace std;

#define MAX_ALLOC 16384

class ts_malloc {
public:
	struct request {
		enum method
		{
			alloc,
			free
		} met;
		void *data;			// pointing to size_t: allocate
							// nothing: free
		int response;	// Point to responsed memory space
	};

	static void init();

	// Server to launch
	static void server();

	// Why do I have to put them here? --- Seemed to be because C doesn't support that
	// Raw require. you'll need it.
	static void require(request *r) {
		req.push(r);
	}

	// Get allocated memory from a repiled request
	static void* get_resp(int id);

	// Require for alloc or free:
	// They'll wait.
	static void* alloc(size_t size) {
		request r;
		r.met = request::method::alloc;
		r.data = (void*)&size;
		r.response = -1;
		require(&r);
		while (r.response < 0) this_thread::yield();
		return get_resp(r.response);
	}

	static void free(void *ptr) {
		request r;
		r.met = request::method::free;
		r.data = ptr;
		require(&r);
	}
	
private:

	static int random_s(void);

	static int get_free();

	static queue<request*> req;
	static map<int, void*> resp;
	static map<void*, int> rel;
};