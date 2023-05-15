// #pragma warning(disable : 6385)

#include <Windows.h>
#include <stdio.h>

#include "utils.h"

#define WCHAR_BUF_SIZ (1024)

#define ERRREPORT()      \
    do {                 \
        hasError = TRUE; \
        goto cleanup;    \
    } while (0)

static BOOL hasError = FALSE;

int wmain(int argc, LPWSTR argv[]) {
#define hBufferSiz (sizeof(HANDLE) * (argc - 1))

    if (argc < 2) {
        eprintf("%s [files to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }

    HANDLE* hPorts = (HANDLE*)malloc(hBufferSiz);
    if (hPorts == NULL) {
        ERRREPORT();
    }
    memset(hPorts, 0, hBufferSiz);

    for (int i = 1; i < argc; ++i) {
        HANDLE hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, i,
                                              0 /* as many as system got*/);

        if (hPort == NULL) {
            printLastError();
            ERRREPORT();
        }
        hPorts[i - 1] = hPort;
    }

    HANDLE* hFiles = (HANDLE*)malloc(hBufferSiz);
    if (hFiles == NULL) {
        ERRREPORT();
    }
    memset(hFiles, 0, hBufferSiz);
    for (int i = 1; i < argc; ++i) {
        LPWSTR path = argv[i];

        HANDLE hFile = CreateFileW(
            path, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE |
                FILE_SHARE_DELETE /* listen on file change, so don't prevent
                                     others from change it */
            ,
            NULL, OPEN_EXISTING /* Opens a file or device, only if it exists. If
                                   the specified file or device does not exist,
                                   the function fails and the last-error code is
                                   set to ERROR_FILE_NOT_FOUND (2). */
            ,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            printLastError();
            ERRREPORT();
        }
        hFiles[i - 1] = hFile;
    }

    for (int i = 1; i < argc; ++i) {
        HANDLE hFile = (HANDLE)hFiles[i - 1];
        if (hFile == NULL) {
            ERRREPORT();
        }
        HANDLE hPort = CreateIoCompletionPort(hFile, NULL, i,
                                              0 /* as many as system got*/);

        if (hPort == NULL) {
            printLastError();
            ERRREPORT();
        }
        hPorts[i - 1] = hPort;
    }

    for (;;) {
    }

cleanup:
    if (hFiles != NULL) {
        for (HANDLE* h = hFiles; h < hFiles + argc - 1; ++h) {
            if (*h != NULL) {
                // ref:
                // https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew#remarks
                // When an application is finished using the object handle
                // returned by CreateFile, use the CloseHandle function to close
                // the handle. This not only frees up system resources, but can
                // have wider influence on things like sharing the file or
                // device and committing data to disk. Specifics are noted
                // within this topic as appropriate.
                CloseHandle(*h);
            }
        }
        free(hFiles);
    }

    if (hasError) {
        return RET_CODE_ERROR;
    }
    return RET_CODE_SUCCESS;
}
