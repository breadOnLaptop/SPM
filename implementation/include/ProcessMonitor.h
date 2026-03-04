#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
    #include <windows.h>
    #include <tlhelp32.h>
    #include <psapi.h>
    #include <sddl.h>
#else
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/resource.h>
    #include <sys/stat.h>
    #include <pwd.h>
    #include <cstdio>
#endif

// Structure representing a process.
struct Process {
    int pid;
    std::string name;
    std::string status;
    int priority;
    double memoryMB;
    std::string owner;
};

#ifdef _WIN32

// Helper: Convert std::wstring to UTF-8 std::string.
inline std::string wstringToUtf8(const std::wstring &wstr) {
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
                                         NULL, 0, NULL, NULL);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(),
                        &strTo[0], sizeNeeded, NULL, NULL);
    return strTo;
}

// Helper: Get owner of the process.
inline std::string getProcessOwner(HANDLE hProcess) {
    HANDLE hToken = NULL;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) return "Unknown";

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(hToken);
        return "Unknown";
    }

    PTOKEN_USER pTokenUser = (PTOKEN_USER)malloc(dwSize);
    if (!pTokenUser) {
        CloseHandle(hToken);
        return "Unknown";
    }

    if (!GetTokenInformation(hToken, TokenUser, pTokenUser, dwSize, &dwSize)) {
        free(pTokenUser);
        CloseHandle(hToken);
        return "Unknown";
    }

    WCHAR szName[256], szDomain[256];
    DWORD dwNameSize = 256, dwDomainSize = 256;
    SID_NAME_USE snu;
    if (!LookupAccountSidW(NULL, pTokenUser->User.Sid, szName, &dwNameSize, szDomain, &dwDomainSize, &snu)) {
        free(pTokenUser);
        CloseHandle(hToken);
        return "Unknown";
    }

    std::string owner = wstringToUtf8(szName);
    free(pTokenUser);
    CloseHandle(hToken);
    return owner;
}

// Overload: getProcessInfo using PROCESSENTRY32.
inline Process getProcessInfo(int pid, const PROCESSENTRY32 &pe32) {
    Process proc;
    proc.pid = pid;
    #if defined(UNICODE) || defined(_UNICODE)
        std::wstring wname(pe32.szExeFile);
        proc.name = wstringToUtf8(wname);
    #else
        proc.name = std::string(pe32.szExeFile);
    #endif

    // Default values.
    proc.priority = 0;
    proc.status = "Unknown";
    proc.memoryMB = 0.0;
    proc.owner = "Unknown";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        DWORD prClass = GetPriorityClass(hProcess);
        if      (prClass == IDLE_PRIORITY_CLASS)           proc.priority = 1;
        else if (prClass == BELOW_NORMAL_PRIORITY_CLASS)   proc.priority = 2;
        else if (prClass == NORMAL_PRIORITY_CLASS)         proc.priority = 3;
        else if (prClass == ABOVE_NORMAL_PRIORITY_CLASS)   proc.priority = 4;
        else if (prClass == HIGH_PRIORITY_CLASS)           proc.priority = 5;
        else                                               proc.priority = 3;
        proc.status = "Running";  // Windows does not expose detailed status via simple API.

        // Get Memory Info.
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            proc.memoryMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
        }

        // Get Owner Info.
        proc.owner = getProcessOwner(hProcess);

        CloseHandle(hProcess);
    }
    return proc;
}

// Overload: getProcessInfo using only pid (searches the snapshot).
inline Process getProcessInfo(int pid) {
    Process proc;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return proc;
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    bool found = false;
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (static_cast<int>(pe32.th32ProcessID) == pid) {
                proc = getProcessInfo(pid, pe32);
                found = true;
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    if (!found) {
        proc.pid = pid;
        proc.name = "Unknown";
        proc.status = "Unknown";
        proc.priority = 0;
    }
    CloseHandle(hProcessSnap);
    return proc;
}

#else // Linux implementation

inline Process getProcessInfo(int pid) {
    Process proc;
    proc.pid = pid;
    proc.memoryMB = 0.0;
    proc.owner = "Unknown";

    // Get Owner
    std::string procPath = "/proc/" + std::to_string(pid);
    struct stat st;
    if (stat(procPath.c_str(), &st) == 0) {
        struct passwd *pw = getpwuid(st.st_uid);
        if (pw) {
            proc.owner = std::string(pw->pw_name);
        }
    }

    // Get Memory from /proc/[pid]/status
    std::string statusPath = procPath + "/status";
    FILE* fs = fopen(statusPath.c_str(), "r");
    if (fs) {
        char line[256];
        while (fgets(line, sizeof(line), fs)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                long rss_kb;
                if (sscanf(line + 6, "%ld", &rss_kb) == 1) {
                    proc.memoryMB = (double)rss_kb / 1024.0;
                }
                break;
            }
        }
        fclose(fs);
    }

    std::string statPath = procPath + "/stat";
    FILE* f = fopen(statPath.c_str(), "r");
    if (!f) {
        proc.name = "Unknown";
        proc.status = "Unknown";
        proc.priority = 0;
        return proc;
    }
    char buffer[1024];
    if (!fgets(buffer, sizeof(buffer), f)) {
        fclose(f);
        proc.name = "Unknown";
        proc.status = "Unknown";
        proc.priority = 0;
        return proc;
    }
    fclose(f);
    std::string line(buffer);
    
    // Extract process name (between parentheses).
    size_t start = line.find('(');
    size_t end = line.find(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        proc.name = line.substr(start + 1, end - start - 1);
    } else {
        proc.name = "Unknown";
    }
    // Process status: character after the closing parenthesis.
    size_t statePos = end + 2;
    if (statePos < line.size()) {
        char stateChar = line[statePos];
        switch (stateChar) {
            case 'R': proc.status = "Running"; break;
            case 'S': proc.status = "Sleeping/Idle"; break;
            case 'T': proc.status = "Stopped"; break;
            case 'Z': proc.status = "Zombie"; break;
            default: proc.status = "Unknown"; break;
        }
    } else {
        proc.status = "Unknown";
    }
    proc.priority = getpriority(PRIO_PROCESS, pid);
    return proc;
}

#endif // _WIN32

// Lists all processes on the system.
inline std::vector<Process> listProcesses() {
    std::vector<Process> processes;
#ifdef _WIN32
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Unable to create process snapshot." << std::endl;
        return processes;
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            int pid = static_cast<int>(pe32.th32ProcessID);
            processes.push_back(getProcessInfo(pid, pe32));
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
#else
    DIR* dir = opendir("/proc");
    if (!dir) {
        std::cerr << "Could not open /proc directory." << std::endl;
        return processes;
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string dirname(entry->d_name);
            if (std::all_of(dirname.begin(), dirname.end(), ::isdigit)) {
                int pid = std::stoi(dirname);
                processes.push_back(getProcessInfo(pid));
            }
        }
    }
    closedir(dir);
#endif
    return processes;
}

#endif // PROCESS_MONITOR_H
