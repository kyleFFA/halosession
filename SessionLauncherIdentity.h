#pragma once
#include <windows.h>
#include <tlhelp32.h>

namespace SessionLauncherIdentity {
inline constexpr wchar_t executableName[] = L"Session.exe";

// Shared by the internal heartbeat and the launcher's offline identity check.
// Fail closed on ambiguous identity rather than watching an unrelated process.
inline DWORD findProcessId() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W entry{sizeof(entry)};
    DWORD result = 0;
    DWORD error = ERROR_NOT_FOUND;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, executableName) == 0) {
                if (result) { result = 0; error = ERROR_DUP_NAME; break; }
                result = entry.th32ProcessID;
                error = ERROR_SUCCESS;
            }
        } while (Process32NextW(snapshot, &entry));
    } else error = GetLastError();
    CloseHandle(snapshot);
    SetLastError(error);
    return result;
}
}
