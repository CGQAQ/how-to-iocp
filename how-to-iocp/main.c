// #pragma warning(disable : 6385)

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <Windows.h>
#include <stdio.h>

#include "utils.h"

#define WCHAR_BUF_SIZ (1024)
#define BUFFER_SIZE (4096)

#define ERRREPORT()      \
    do {                 \
        hasError = TRUE; \
        goto cleanup;    \
    } while (0)

static BOOL hasError = FALSE;

typedef struct {
    OVERLAPPED overlapped;
    HANDLE directoryHandle;
    CHAR buffer[BUFFER_SIZE];
} FileData;

DWORD WINAPI CompletionThread(LPVOID lpParam) {
    HANDLE hPort = (HANDLE)lpParam;

    DWORD bytesTransferred;
    ULONG_PTR completionKey;
    LPOVERLAPPED overlapped;

    for (;;) {
        BOOL status = GetQueuedCompletionStatus(
            hPort, &bytesTransferred, &completionKey, &overlapped, 500);

        if (!status) {
            int err = GetLastError();

            if (err == WAIT_TIMEOUT) {
                LOG(L"TIMEOUT\n");
                continue;
            }

            unreachable("GetQueuedCompletionStatus error");
        }

        FileData* fileData = (FileData*)overlapped;

        // Cast the buffer to the correct structure type
        FILE_NOTIFY_INFORMATION* fileInfo =
            (FILE_NOTIFY_INFORMATION*)fileData->buffer;

        while (1) {
            // Print the file name that was changed
            wchar_t fileName[MAX_PATH];
            memcpy(fileName, fileInfo->FileName, fileInfo->FileNameLength);
            fileName[fileInfo->FileNameLength / sizeof(wchar_t)] = '\0';

            switch (fileInfo->Action) {
                case FILE_ACTION_ADDED:
                    wprintf(L"File changed(FILE_ACTION_ADDED): %s\n", fileName);
                    break;
                case FILE_ACTION_REMOVED:
                    wprintf(L"File changed(FILE_ACTION_REMOVED): %s\n",
                            fileName);
                    break;
                case FILE_ACTION_MODIFIED:
                    wprintf(L"File changed(FILE_ACTION_MODIFIED): %s\n",
                            fileName);
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                    wprintf(L"File changed(FILE_ACTION_RENAMED_OLD_NAME): %s\n",
                            fileName);
                    break;
                case FILE_ACTION_RENAMED_NEW_NAME:
                    wprintf(L"File changed(FILE_ACTION_RENAMED_NEW_NAME): %s\n",
                            fileName);
                    break;
                default:
                    eprintf("Unknown fileInfo#action %u", fileInfo->Action);
            }

            // Check if there are more changes in the buffer
            if (fileInfo->NextEntryOffset == 0) break;

            // Move to the next entry in the buffer
            fileInfo = (FILE_NOTIFY_INFORMATION*)((BYTE*)fileInfo +
                                                  fileInfo->NextEntryOffset);
        }
    }
}

int wmain(int argc, LPWSTR argv[]) {
#define hBufferSiz (sizeof(HANDLE) * (argc - 1))
    HANDLE* hDirs = NULL;
    FileData** fileDatas = NULL;

    if (argc < 2) {
        eprintf("%s [...directories to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }
    HANDLE hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0,
                                          0 /* as many as system got*/);
    if (hPort == NULL) {
        printLastError();
        ERRREPORT();
    }

    hDirs = (HANDLE*)malloc(hBufferSiz);
    if (hDirs == NULL) {
        ERRREPORT();
    }
    memset(hDirs, 0, hBufferSiz);
    for (int i = 1; i < argc; ++i) {
        LPWSTR path = argv[i];

        if (!PathIsDirectoryW(path)) {
            eprintf("Arguments can only be directories!");
            ERRREPORT();
        }

        WCHAR pathBuf[MAX_PATH];
        DWORD len = GetFullPathNameW(path, MAX_PATH, pathBuf, NULL);
        if (len == 0 || len > MAX_PATH - 1) {
            eprintf("GETFULLNAME failed(%s)", path);
        }
        wprintf_s(L"path is: %s\n", pathBuf);
        pathBuf[len] = 0;

        HANDLE hFile = CreateFileW(
            pathBuf, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE, /* listen on file change, so
                  don't prevent others from change it */
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

        LOG(L"\"%s\" listened", (LPWSTR)pathBuf);
        hDirs[i - 1] = hFile;

        HANDLE hPort2 = CreateIoCompletionPort(hFile, hPort, (ULONG_PTR)hFile,
                                               0 /* as many as system got*/);
        if (hPort == NULL || hPort != hPort2) {
            printLastError();
            ERRREPORT();
        }
    }

    HANDLE thread = CreateThread(NULL, 0, CompletionThread, hPort, 0, NULL);

#define BUF_SIZ (sizeof(FileData*) * (argc - 1))
    fileDatas = (FileData**)malloc(BUF_SIZ);
    ZeroMemory(fileDatas, BUF_SIZ);
#undef BUF_SIZ
#define BUF_SIZ (sizeof(FileData))
    for (int i = 1; i < argc; ++i) {
        fileDatas[i - 1] = (FileData*)malloc(BUF_SIZ);
        ZeroMemory(fileDatas[i - 1], BUF_SIZ);
    }

    for (;;) {
        for (int i = 1; i < argc; ++i) {
            HANDLE hDir = hDirs[i - 1];
            FileData* data = fileDatas[i - 1];
            ZeroMemory(data, BUF_SIZ);
            ZeroMemory(&(data->overlapped), sizeof(OVERLAPPED));
            data->directoryHandle = hDir;
            fileDatas[i - 1] = data;

            DWORD bytesRead;
            if (ReadDirectoryChangesW(
                    hDir, data->buffer, BUFFER_SIZE, TRUE,
                    FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                        FILE_NOTIFY_CHANGE_ATTRIBUTES |
                        FILE_NOTIFY_CHANGE_SIZE |
                        FILE_NOTIFY_CHANGE_LAST_WRITE |
                        FILE_NOTIFY_CHANGE_LAST_ACCESS |
                        FILE_NOTIFY_CHANGE_CREATION |
                        FILE_NOTIFY_CHANGE_SECURITY,
                    &bytesRead, &(data->overlapped), NULL) == 0) {
                fprintf(stderr, "Failed to start ReadDirectoryChangesW: %lu\n",
                        GetLastError());
                continue;
            }
        }
        Sleep(500);
    }
#undef BUF_SIZ

cleanup:
    if (hDirs != NULL) {
        for (HANDLE* h = hDirs; h < hDirs + argc - 1; ++h) {
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
        free(hDirs);
    }

    if (fileDatas != NULL) {
        free(fileDatas);
    }

    if (hasError) {
        return RET_CODE_ERROR;
    }
    return RET_CODE_SUCCESS;
}
