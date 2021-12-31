#pragma once

// For memory leak debugger

#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#ifndef _new
#define _new new (_NORMAL_BLOCK, __FILE__, __LINE__ )
#define new _new
#endif