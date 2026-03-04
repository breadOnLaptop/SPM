#ifndef SYSTEM_METRICS_H
#define SYSTEM_METRICS_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/sysinfo.h>
    #include <unistd.h>
#endif

class SystemMetrics {
public:
    SystemMetrics() {
#ifdef _WIN32
        prev_idle_time_ = {0};
        prev_kernel_time_ = {0};
        prev_user_time_ = {0};
        getCPUUsage(); // Initial call to prime the delta
#else
        prev_user_ = prev_nice_ = prev_system_ = prev_idle_ = 0;
        getCPUUsage();
#endif
    }

    // Returns CPU usage as a percentage (0.0 - 100.0)
    double getCPUUsage() {
#ifdef _WIN32
        FILETIME idle_time, kernel_time, user_time;
        if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) return 0.0;

        auto ftToUint64 = [](const FILETIME& ft) {
            return (uint64_t(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        };

        uint64_t idle = ftToUint64(idle_time);
        uint64_t kernel = ftToUint64(kernel_time);
        uint64_t user = ftToUint64(user_time);

        uint64_t idle_delta = idle - ftToUint64(prev_idle_time_);
        uint64_t kernel_delta = kernel - ftToUint64(prev_kernel_time_);
        uint64_t user_delta = user - ftToUint64(prev_user_time_);
        uint64_t total_delta = kernel_delta + user_delta;

        prev_idle_time_ = idle_time;
        prev_kernel_time_ = kernel_time;
        prev_user_time_ = user_time;

        if (total_delta == 0) return 0.0;
        return (double(total_delta - idle_delta) * 100.0) / total_delta;

#else // Linux
        std::ifstream file("/proc/stat");
        std::string cpu;
        uint64_t user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
        if (!(file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice)) return 0.0;

        uint64_t total_idle = idle + iowait;
        uint64_t non_idle = user + nice + system + irq + softirq + steal;
        uint64_t total = total_idle + non_idle;

        uint64_t total_delta = total - (prev_user_ + prev_nice_ + prev_system_ + prev_idle_);
        uint64_t idle_delta = total_idle - prev_idle_;

        prev_user_ = user; prev_nice_ = nice; prev_system_ = system; prev_idle_ = total_idle;

        if (total_delta == 0) return 0.0;
        return (double(total_delta - idle_delta) * 100.0) / total_delta;
#endif
    }

    // Returns RAM usage as a percentage (0.0 - 100.0)
    double getRAMUsage() {
#ifdef _WIN32
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        if (GlobalMemoryStatusEx(&memInfo)) {
            return (double)memInfo.dwMemoryLoad;
        }
        return 0.0;
#else // Linux
        struct sysinfo memInfo;
        if (sysinfo(&memInfo) == 0) {
            unsigned long total = memInfo.totalram * memInfo.mem_unit;
            unsigned long free = memInfo.freeram * memInfo.mem_unit;
            return ((double)(total - free) / (double)total) * 100.0;
        }
        return 0.0;
#endif
    }

private:
#ifdef _WIN32
    FILETIME prev_idle_time_, prev_kernel_time_, prev_user_time_;
#else
    uint64_t prev_user_, prev_nice_, prev_system_, prev_idle_;
#endif
};

#endif // SYSTEM_METRICS_H
