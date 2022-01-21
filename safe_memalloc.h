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

	static void init() {
		resp.clear();
		thread t = thread(server);
		t.detach();
	}

	// Server to launch
	static void server() {
		while (true) {
			while (!req.empty()) {
				request *q = req.front();
				req.pop();
				switch (q->met) {
				case ts_malloc::request::method::alloc:
					size_t s = *((size_t*)q->data);
					int r = get_free();
					resp[r] = (void*) new char[s];
					q->response = r;
					break;
				case ts_malloc::request::method::free:
					delete[] q->data;
				default:
					throw bad_function_call();
				}
			}
			this_thread::yield();
		}
	}

	// Raw require. you'll need it.
	static void require(request *r) {
		req.push(r);
	}

	static void* get_resp(int id) {
		return resp[id];
	}

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

	static int random_s(void) {
		static bool firstrun = true;
		if (firstrun) {
			srand(time(NULL));
			firstrun = false;
		}
		return rand();
	}

	static int get_free() {
#ifndef _SMAL_NOT_LIMITED_
		if (resp.size() > MAX_ALLOC) {
			throw bad_alloc();
		}
#endif

		int r;
		do {
			r = random_s();
		} while (resp.count(r));
		return r;
	}

	static queue<request*> req;
	static map<int, void*> resp;
};