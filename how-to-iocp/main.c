#include <Windows.h>
#include <stdio.h>

#include "utils.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        eprintf("%s [files to listen]\n", argv[0]);
        return RET_CODE_ERROR;
    }

    HANDLE* handles = (HANDLE)malloc(sizeof(HANDLE) * (argc - 1));
    for (int i = 1; i < argc; ++i) {
        // GetFileAttributes()
        // CreateIoCompletionPort();
    }

    free(handles);

    return 0;
}
