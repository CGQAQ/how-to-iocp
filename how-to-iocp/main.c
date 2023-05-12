#include <Windows.h>
#include <stdio.h>

#include "utils.h"

#define WCHAR_BUF_SIZ (1024)

int main(int argc, char* argv[]) {
    LPCWCHAR wchar_buf = malloc(WCHAR_BUF_SIZ * sizeof(WCHAR));

    if (argc < 2) {
        eprintf("%s [files to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }

    HANDLE* hFiles = (HANDLE)malloc(sizeof(HANDLE) * (argc - 1));
    memset(hFiles, 0, 0);
    for (int i = 1; i < argc; ++i) {
        const char* path = argv[i];

        int buffer_size = MultiByteToWideChar(
            CP_ACP, MB_PRECOMPOSED, path, -1 /* null-terminated string */,
            wchar_buf, WCHAR_BUF_SIZ /* get size */);

        if (buffer_size == 0) {
            DWORD err = GetLastError();
            switch (err) {
                case ERROR_INSUFFICIENT_BUFFER:
                    break;
                case ERROR_INVALID_FLAGS:
                    break;
                case ERROR_INVALID_PARAMETER:
                    break;
                case ERROR_NO_UNICODE_TRANSLATION:
                    break;

                default:
                    unreachable(
                        "MultiByteToWideChar erroring an error that is not "
                        "defined");
                    break;
            }
        }

        DWORD attr = GetFileAttributes(path);
        if (attr == INVALID_FILE_ATTRIBUTES) {
            printLastError();
        }

        // CreateIoCompletionPort();
    }

    for (HANDLE* h = hFiles; h < hFiles + argc - 1; ++h) {
        if (*h != NULL) {
        }
    }
    free(hFiles);

    return 0;
}
