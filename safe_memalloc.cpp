#include "safe_memalloc.h"

// To not allocate twice or more
queue<ts_malloc::request*> ts_malloc::req;
map<int, void*> ts_malloc::resp;