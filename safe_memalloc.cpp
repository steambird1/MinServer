#include "safe_memalloc.h"

void ts_malloc::init()
{
	if (initalized) return;
	initalized = true;
	//resp.clear();
	thread t = thread(server);
	t.detach();
}

void ts_malloc::server()
{
	while (true) {
		self_lock();
		while (!req.empty()) {
			request *q = req.front();
			req.pop();
			size_t s;
			int r;
			switch (q->met) {
			case ts_malloc::request::method::alloc:
				s = *((size_t*)q->data);
				r = get_free();
				resp[r] = (void*) new char[s];
				rel[resp[r]] = r;
				q->response = r;
				break;
			case ts_malloc::request::method::free:
				resp.erase(rel[q->data]);
				rel.erase(q->data);
				delete[] q->data;
			default:
				throw bad_function_call();
			}
		}
		m.unlock();
		this_thread::yield();
	}
}

void * ts_malloc::get_resp(int id)
{
	self_lock();
	void *res = resp[id];
	m.unlock();
	return res;
}

int ts_malloc::random_s(void)
{
	static bool firstrun = true;
	if (firstrun) {
		srand(time(NULL));
		firstrun = false;
	}
	return rand();
}

int ts_malloc::get_free()
{
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

void ts_malloc::self_lock()
{
	/*while (!m.try_lock()) {
		// Just for better debug
		this_thread::yield();
	}*/	// "Trylock not supported"
	while (true) {
		try {
			m.lock();
		}
		catch (...) {
			//...
		}
		this_thread::yield();
	}
}

queue<ts_malloc::request*> ts_malloc::req;
map<int, void*> ts_malloc::resp;
map<void*, int> ts_malloc::rel;
bool ts_malloc::initalized;
mutex ts_malloc::m;