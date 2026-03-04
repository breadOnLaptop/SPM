#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <thread>
#include <chrono>
#include "../include/ProcessMonitor.h"
#include "../include/ProcessControl.h"
#include "../include/Logger.h"
#include "../include/Formatter.h"

using namespace std;

// Global snapshot manager.
ProcessSnapshot g_Snapshot;

// Robust integer input helper.
int getSafeInt(const string& prompt = "Enter choice: ") {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        } else {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "[ERROR] Invalid input. Please enter a valid number." << endl;
        }
    }
}

int main() {
    int choice;
    int pid, newPriority;
    
    // Initialize loggers.
    ImplementationLogger::init("logs/implementation_log.txt");
    ProcessLogger::init("logs/process_log.txt");
    ImplementationLogger::log(ImplementationLogger::INFO, "SPM started.");

    // Initial refresh to populate the snapshot.
    g_Snapshot.refresh();
    
    while (true) {
        cout << "\n=== Smart Process Manager (SPM) ===" << endl;
        cout << "1. List Processes (from Snapshot)" << endl;
        cout << "2. Refresh Snapshot" << endl;
        cout << "3. Kill Process" << endl;
        cout << "4. Change Process Priority" << endl;
        cout << "5. Update Process Log" << endl;
        cout << "6. Exit" << endl;
        
        choice = getSafeInt("Enter your choice: ");
        
        switch (choice) {
            case 1: {
                ImplementationLogger::log(ImplementationLogger::INFO, "Listing processes from snapshot.");
                auto processes = g_Snapshot.getProcesses();
                cout << Formatter::formatProcessTable(processes);
                break;
            }
            case 2: {
                cout << "Refreshing..." << endl;
                g_Snapshot.refresh();
                cout << "Snapshot updated." << endl;
                break;
            }
            case 3: {
                pid = getSafeInt("Enter PID to kill: ");
                if (killProcess(pid)) {
                    cout << "Process " << pid << " termination signal sent." << endl;
                    ImplementationLogger::log(ImplementationLogger::INFO, "Killed process with PID " + to_string(pid));
                } else {
                    cout << "[ERROR] Failed to kill process " << pid << "." << endl;
                    ImplementationLogger::log(ImplementationLogger::LOG_ERROR, "Failed to kill process with PID " + to_string(pid));
                }
                break;
            }
            case 4: {
                pid = getSafeInt("Enter PID to change priority: ");
                auto processes = g_Snapshot.getProcesses();
                auto it = std::find_if(processes.begin(), processes.end(), [pid](const Process &p) {
                    return p.pid == pid;
                });
                if (it == processes.end()) {
                    cout << "[ERROR] Process ID " << pid << " not found in current snapshot." << endl;
                    break;
                }
                
                newPriority = getSafeInt("Enter desired priority (Win: 1-5, Linux: -20 to 19): ");
                if (changeProcessPriority(pid, newPriority)) {
                    cout << "Priority change requested." << endl;
                } else {
                    cout << "[ERROR] Failed to change priority." << endl;
                }
                break;
            }
            case 5: {
                auto processes = g_Snapshot.getProcesses();
                ProcessLogger::update(processes);
                cout << "Process log updated." << endl;
                break;
            }
            case 6: {
                ImplementationLogger::log(ImplementationLogger::INFO, "Exiting SPM.");
                goto exit_loop;
            }
            default: {
                cout << "Invalid choice! Try again." << endl;
                break;
            }
        }
    }
exit_loop:
    ImplementationLogger::close();
    ProcessLogger::close();
    return 0;
}
