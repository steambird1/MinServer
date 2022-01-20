
#ifdef APPLICATIONLAUNCHER_EXPORTS
#define APPLICATIONLAUNCHER_API __declspec(dllexport)
#else
#define APPLICATIONLAUNCHER_API __declspec(dllimport)
#endif

#include "../c_framework.h"
#include <cstdlib>
#include <ctime>
using namespace std;

#define mmalloc(...) as->mc_lib.m_alloc(__VA_ARGS__)
#define mfree(...) as->mc_lib.m_free(__VA_ARGS__)

#define itoa _itoa