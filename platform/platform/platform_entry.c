

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <Tchar.h>
#include <strsafe.h>

#include "platform/win32/asset_sys_win32.h"

#include "win32/platform_win32.c"
#include "platform/win32/asset_sys_win32.c"
#include "platform/globals.c"

#elif defined(linux) || defined(__unix__)

#include "linux/platform_linux.c"

#else

#error Unsupported operating system!

#endif
