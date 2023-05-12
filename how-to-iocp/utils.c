#include "utils.h"

#include <Windows.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

void printLastError() {
    LPSTR lpBuffer = NULL;

    DWORD error_code = GetLastError();

    DWORD msg_len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&lpBuffer,
        0, NULL);

    if (msg_len == 0) {
        printf_s("Failed to retrieve error message.\n");
    } else {
        printf_s("Error Message(%d): %s", error_code, lpBuffer);
    }

    LocalFree(lpBuffer);
}

// print to stderr
void eprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}

void unreachable(const char* errmsg) {
    assert(errmsg != NULL);
    eprintf("This should never happened %s", errmsg);
    exit(RET_CODE_ERROR);
}
