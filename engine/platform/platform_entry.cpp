

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)

#include "ui/win32/imgui_impl_win32.cpp"

#include "ui/win32/imgui_dx11.cpp"

#include "win32/platform_win32.cpp"

#elif defined(linux) || defined(__unix__)

#include "linux/platform_linux.c"

#else

#error Unsupported operating system!

#endif
