#include <iostream>
#include <Windows.h>

int main()
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wchar_t cmdLine[] =
        L"C:\\221-331_Sinkovskiy\\LR1\\build\\Desktop_Qt_6_10_2_MSVC2022_64bit-Debug\\LR_1.exe";

    const wchar_t* currentDir =
        L"C:\\221-331_Sinkovskiy\\LR1\\build\\Desktop_Qt_6_10_2_MSVC2022_64bit-Debug";

    BOOL created = CreateProcessW(
        nullptr,        // lpApplicationName
        cmdLine,        // lpCommandLine (изменяемый буфер)
        nullptr,        // lpProcessAttributes
        nullptr,        // lpThreadAttributes
        FALSE,          // bInheritHandles
        0,              // dwCreationFlags
        nullptr,        // lpEnvironment
        currentDir,     // lpCurrentDirectory
        &si,            // lpStartupInfo
        &pi             // lpProcessInformation
    );

    if (created) {
        std::cout << "***CreateProcessW() success!" << std::endl;
        std::cout << "***CreateProcessW() pid = " << std::dec << pi.dwProcessId << std::endl;
    }
    else {
        DWORD lastError = GetLastError();
        std::cout << "***CreateProcessW() failed! GetLastError() = "
            << std::dec << lastError << std::endl;
        return 1;
    }

    bool isAttached = DebugActiveProcess(pi.dwProcessId);
    if (!isAttached) {
        DWORD lastError = GetLastError();
        std::cout << "***DebugActiveProcess() FAILED, GetLastError() = "
            << std::dec << lastError << std::endl;

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 1;
    }
    else {
        std::cout << "***DebugActiveProcess() success!" << std::endl;
    }

    DEBUG_EVENT debugEvent;
    while (true) {
        bool result1 = WaitForDebugEvent(&debugEvent, INFINITE);
        if (!result1) {
            std::cout << "***WaitForDebugEvent() failed!" << std::endl;
            break;
        }

        bool result2 = ContinueDebugEvent(
            debugEvent.dwProcessId,
            debugEvent.dwThreadId,
            DBG_CONTINUE
        );

        if (!result2) {
            std::cout << "***ContinueDebugEvent() failed!" << std::endl;
            break;
        }
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}