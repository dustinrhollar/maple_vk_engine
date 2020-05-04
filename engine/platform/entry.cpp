
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "win32/platform_win32.cpp"
#elif defined(linux) || defined(__unix__)
#include "linux/platform_x11.cpp"
#else
#error Undefined operating system!
#endif