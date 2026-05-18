#ifndef PROCESS_CONTROL_H
#define PROCESS_CONTROL_H

#include <iostream>
#include <chrono>
#include <thread>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <signal.h>
    #include <sys/resource.h>
    #include <unistd.h>
#endif

#ifdef _WIN32
// Helper callback for Windows to send WM_CLOSE to windows belonging to a PID.
struct TerminateData {
    DWORD dwProcessId;
    bool bSentClose;
};

inline BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam) {
    TerminateData* pData = reinterpret_cast<TerminateData*>(lParam);
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwProcessId == pData->dwProcessId) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        pData->bSentClose = true;
    }
    return TRUE;
}
#endif

// Kills the process with the given PID using a tiered strategy.
inline bool killProcess(int pid) {
#ifdef _WIN32
    // 1. Attempt graceful close via WM_CLOSE if it has windows.
    TerminateData data = { (DWORD)pid, false };
    EnumWindows(TerminateAppEnum, reinterpret_cast<LPARAM>(&data));

    // Wait a bit for the app to close.
    if (data.bSentClose) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // 2. Check if process still exists and use TerminateProcess if necessary.
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (!hProcess) {
        // If we can't open it, it might already be dead.
        return true; 
    }

    // Check if it's already exited.
    DWORD dwExitCode = 0;
    if (GetExitCodeProcess(hProcess, &dwExitCode) && dwExitCode != STILL_ACTIVE) {
        CloseHandle(hProcess);
        return true;
    }

    // Hard kill.
    bool result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
#else
    // 1. Try SIGTERM first (graceful).
    if (kill(pid, SIGTERM) == 0) {
        // Wait a bit.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Check if still alive.
        if (kill(pid, 0) != 0) {
            return true; // Successfully closed.
        }
    }

    // 2. Fall back to SIGKILL (hard kill).
    return (kill(pid, SIGKILL) == 0);
#endif
}

// Changes the process's priority.
// On Windows, newPriority: 1 (Idle) to 5 (High). On Linux, newPriority is the nice value.
inline bool changeProcessPriority(int pid, int newPriority) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!hProcess) {
        std::cerr << "Failed to open process for priority change." << std::endl;
        return false;
    }
    int priorityClass;
    switch (newPriority) {
        case 1: priorityClass = IDLE_PRIORITY_CLASS; break;
        case 2: priorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
        case 3: priorityClass = NORMAL_PRIORITY_CLASS; break;
        case 4: priorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
        case 5: priorityClass = HIGH_PRIORITY_CLASS; break;
        default: priorityClass = NORMAL_PRIORITY_CLASS;
    }
    bool success = SetPriorityClass(hProcess, priorityClass);
    CloseHandle(hProcess);
    return success;
#else
    return (setpriority(PRIO_PROCESS, pid, newPriority) == 0);
#endif
}

#endif // PROCESS_CONTROL_H
