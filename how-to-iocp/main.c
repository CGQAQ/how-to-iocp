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
    if (argc < 2) {
        eprintf("%s [files to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }

    HANDLE* hFiles = (HANDLE*)malloc(sizeof(HANDLE) * (argc - 1));
    if (hFiles == NULL) {
        ERRREPORT();
    }

    memset(hFiles, 0, sizeof(HANDLE) * (argc - 1));
    for (int i = 1; i < argc; ++i) {
        LPWSTR path = argv[i];

        HANDLE file = CreateFileW(
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
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE) {
            printLastError();
            ERRREPORT();
        }
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
