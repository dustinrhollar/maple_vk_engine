

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#error Windows is not currently supported on the C/OpenGl rework!
#include "win32/platform_win32.c"

#elif defined(linux) || defined(__unix__)

#include "linux/platform_linux.c"

#else

#error Unsupported operating system!

#endif