
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <atomic>

enum class style {
    reset     = 0,
    bold      = 1,
    dim       = 2,
    italic    = 3,
    underline = 4,
    blink     = 5,
    rblink    = 6,
    reversed  = 7,
    conceal   = 8,
    crossed   = 9
};

enum class fg {
    black        = 0,
    blue         = 1,
    green        = 2,
    cyan         = 3,
    red          = 4,
    magenta      = 5,
    brown        = 6,
    lightgray    = 7,
    gray         = 8,
    lightblue    = 9,
    lightgreen   = 10,
    lightcyan    = 11,
    lightred     = 12,
    lightmagenta = 13,
    yellow       = 14,
    white        = 15,
    reset        = black
};

enum class bg {
    navyblue = 16,
    green    = 32,
    teal     = 48,
    maroon   = 64,
    purple   = 80,
    olive    = 96,
    silver   = 112,
    gray     = 128,
    blue     = 144,
    lime     = 160,
    cyan     = 176,
    red      = 192,
    magenta  = 208,
    yellow   = 224,
    white    = 240,
    reset    = white
};

enum class fgB {
    black   = 90,
    red     = 91,
    green   = 92,
    yellow  = 93,
    blue    = 94,
    magenta = 95,
    cyan    = 96,
    gray    = 97
};

enum class bgB {
    black   = 100,
    red     = 101,
    green   = 102,
    yellow  = 103,
    blue    = 104,
    magenta = 105,
    cyan    = 106,
    gray    = 107
};

// Redirect IO to a Console.
void InitializeLogger();

/*

@Source https://github.com/agauniyal/rang/blob/master/include/rang.hpp

*/

template <typename T>
using enableStd = typename std::enable_if<
std::is_same<T, style>::value || std::is_same<T, fg>::value
|| std::is_same<T, bg>::value || std::is_same<T, fgB>::value
|| std::is_same<T, bgB>::value,
std::ostream &>::type;

template <typename T>
inline enableStd<T> operator<<(std::ostream &os, const T value)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, (WORD)value);
    return os;
}

#endif // _LOGGER_H_