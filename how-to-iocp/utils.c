#include "utils.h"

#include <Windows.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

void _printLastError() {
    LPWSTR lpBuffer = NULL;

    DWORD error_code = GetLastError();

    DWORD msg_len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&lpBuffer, 0, NULL);

    if (msg_len == 0) {
        wprintf_s(L"Failed to retrieve error message.\n");
    } else {
        wprintf_s(L"Error Message(%d): %s", error_code, lpBuffer);
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
