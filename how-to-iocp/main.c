// #pragma warning(disable : 6385)

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
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

DWORD WINAPI CompletionThread(LPVOID lpParam) {
    HANDLE completionPort = (HANDLE)lpParam;

    while (1) {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        LPOVERLAPPED overlapped;

        // Wait for completion
        if (!GetQueuedCompletionStatus(completionPort, &bytesTransferred,
                                       &completionKey, &overlapped, INFINITE)) {
            printf("Failed to get completion status: %d\n", GetLastError());
            return 1;
        }

        HANDLE changedFileHandle = (HANDLE)completionKey;

        WCHAR path[MAX_PATH];
        DWORD len = GetFinalPathNameByHandleW(changedFileHandle, path, MAX_PATH,
                                              FILE_NAME_NORMALIZED);

        if (len == 0 || len > MAX_PATH - 1) {
            fwprintf_s(stderr, L"failed to retrieve path");
            return 1;
        }

        path[len] = 0;
        LOG(L"\"%s\" changed", (LPWSTR)path);
    }
    return 0;
}

int wmain(int argc, LPWSTR argv[]) {
#define hBufferSiz (sizeof(HANDLE) * (argc - 1))
    HANDLE* hPorts = NULL;
    HANDLE* hFiles = NULL;
    HANDLE* threads = NULL;

    if (argc < 2) {
        eprintf("%s [files to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }

    hPorts = (HANDLE*)malloc(hBufferSiz);
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

    hFiles = (HANDLE*)malloc(hBufferSiz);
    if (hFiles == NULL) {
        ERRREPORT();
    }
    memset(hFiles, 0, hBufferSiz);
    for (int i = 1; i < argc; ++i) {
        LPWSTR path = argv[i];

        WCHAR pathBuf[MAX_PATH];
        if (GetFullPathNameW(path, MAX_PATH, pathBuf, NULL)) {
            wprintf_s(L"path is: %s\n", pathBuf);
        }

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

        WCHAR realPath[MAX_PATH];
        DWORD len = GetFinalPathNameByHandleW(hFile, realPath, MAX_PATH,
                                              FILE_NAME_NORMALIZED);

        if (len == 0 || len > MAX_PATH - 1) {
            fwprintf_s(stderr, L"failed to retrieve path");
            return 1;
        }

        realPath[len] = 0;
        LOG(L"\"%s\" listened", (LPWSTR)realPath);

        hFiles[i - 1] = hFile;
    }

    for (int i = 1; i < argc; ++i) {
        HANDLE hFile = (HANDLE)hFiles[i - 1];
        HANDLE hPort = (HANDLE)hPorts[i - 1];

        if (hFile == NULL) {
            ERRREPORT();
        }
        HANDLE hPort2 = CreateIoCompletionPort(hFile, hPort, (ULONG_PTR)hFile,
                                               0 /* as many as system got*/);

        if (hPort == NULL || hPort != hPort2) {
            printLastError();
            ERRREPORT();
        }
    }

    threads = (HANDLE*)malloc(hBufferSiz);
    if (threads == NULL) {
        ERRREPORT();
    }
    memset(threads, 0, hBufferSiz);
    for (int i = 1; i < argc; ++i) {
        HANDLE hPort = hPorts[i - 1];
        threads[i - 1] =
            CreateThread(NULL, 0, CompletionThread, hPort, 0, NULL);
    }

    WaitForMultipleObjects(argc - 1, threads, TRUE, INFINITE);

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
