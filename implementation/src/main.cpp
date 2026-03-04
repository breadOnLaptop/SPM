#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include "../include/ProcessMonitor.h"
#include "../include/ProcessControl.h"
#include "../include/Logger.h"

using namespace std;

// Robust integer input helper.
int getSafeInt(const string& prompt = "Enter choice: ") {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            // Success, consume the newline character left in the buffer.
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        } else {
            // Failure, clear error state and discard invalid input.
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "[ERROR] Invalid input. Please enter a valid number." << endl;
        }
    }
}

// Helper function to center align a string.
string centerString(const string &s, int width) {
    int len = s.length();
    if (width <= len) return s;
    int pad = width - len;
    int padLeft = pad / 2;
    int padRight = pad - padLeft;
    return string(padLeft, ' ') + s + string(padRight, ' ');
}

// Helper function to print a horizontal line.
void printHorizontalLine(int pidWidth, int nameWidth, int statusWidth, int priorityWidth, int memoryWidth, int ownerWidth) {
    cout << "+" 
         << string(pidWidth, '-') << "+"
         << string(nameWidth, '-') << "+"
         << string(statusWidth, '-') << "+"
         << string(priorityWidth, '-') << "+"
         << string(memoryWidth, '-') << "+"
         << string(ownerWidth, '-') << "+"
         << endl;
}

// Prints the process table to the console with table borders.
void printProcesses(const vector<Process>& processes, int pidWidth = 7, int nameWidth = 20, int statusWidth = 15, int priorityWidth = 8, int memoryWidth = 12, int ownerWidth = 15) {
    // Print header.
    printHorizontalLine(pidWidth, nameWidth, statusWidth, priorityWidth, memoryWidth, ownerWidth);
    cout << "|" << centerString("PID", pidWidth)
         << "|" << centerString("Name", nameWidth)
         << "|" << centerString("Status", statusWidth)
         << "|" << centerString("Priority", priorityWidth)
         << "|" << centerString("Mem (MB)", memoryWidth)
         << "|" << centerString("Owner", ownerWidth)
         << "|" << endl;
    printHorizontalLine(pidWidth, nameWidth, statusWidth, priorityWidth, memoryWidth, ownerWidth);

    // Print each process row.
    for (const auto& proc : processes) {
        string displayName = proc.name;
        if (displayName.length() > nameWidth) {
            displayName = displayName.substr(0, nameWidth - 3) + "...";
        }

        stringstream memStream;
        memStream << fixed << setprecision(2) << proc.memoryMB;

        cout << "|" << centerString(to_string(proc.pid), pidWidth)
             << "|" << centerString(displayName, nameWidth)
             << "|" << centerString(proc.status, statusWidth)
             << "|" << centerString(to_string(proc.priority), priorityWidth)
             << "|" << centerString(memStream.str(), memoryWidth)
             << "|" << centerString(proc.owner, ownerWidth)
             << "|" << endl;
    }
    printHorizontalLine(pidWidth, nameWidth, statusWidth, priorityWidth, memoryWidth, ownerWidth);
}

int main() {
    int choice;
    int pid, newPriority;
    
    // Initialize loggers.
    ImplementationLogger::init("logs/implementation_log.txt");
    ProcessLogger::init("logs/process_log.txt");
    ImplementationLogger::log(ImplementationLogger::INFO, "SPM started.");
    
    while (true) {
        cout << "\n=== Smart Process Manager (SPM) ===" << endl;
        cout << "1. List Processes" << endl;
        cout << "2. Kill Process" << endl;
        cout << "3. Change Process Priority" << endl;
        cout << "4. Update Process Log" << endl;
        cout << "5. Exit" << endl;
        
        choice = getSafeInt("Enter your choice: ");
        
        switch (choice) {
            case 1: {
                ImplementationLogger::log(ImplementationLogger::INFO, "Listing processes.");
                auto processes = listProcesses();
                printProcesses(processes);
                break;
            }
            case 2: {
                pid = getSafeInt("Enter PID to kill: ");
                if (killProcess(pid)) {
                    cout << "Process " << pid << " killed successfully." << endl;
                    ImplementationLogger::log(ImplementationLogger::INFO, "Killed process with PID " + to_string(pid));
                } else {
                    cout << "[ERROR] Failed to kill process " << pid << ". Ensure you have sufficient permissions." << endl;
                    ImplementationLogger::log(ImplementationLogger::LOG_ERROR, "Failed to kill process with PID " + to_string(pid));
                }
                break;
            }
            case 3: {
                pid = getSafeInt("Enter PID to change priority: ");
                
                // Retrieve current processes to check if PID exists.
                auto processes = listProcesses();
                auto it = std::find_if(processes.begin(), processes.end(), [pid](const Process &p) {
                    return p.pid == pid;
                });
                if (it == processes.end()) {
                    cout << "[ERROR] Process ID " << pid << " not found. Please enter a valid PID." << endl;
                    ImplementationLogger::log(ImplementationLogger::LOG_ERROR, "Attempted to change priority for invalid PID " + to_string(pid));
                    break;
                }
                
                Process beforeChange = getProcessInfo(pid);
                cout << "Current priority for process " << pid << ": " << beforeChange.priority << endl;
                cout << "For Windows, use 1 (Idle) to 5 (High)." << endl;
                cout << "For Linux, enter desired nice value (-20 to 19)." << endl;
                newPriority = getSafeInt("Enter desired priority: ");

                if (changeProcessPriority(pid, newPriority)) {
                    Process afterChange = getProcessInfo(pid);
                    cout << "Priority changed successfully for process " << pid << "." << endl;
                    ImplementationLogger::log(ImplementationLogger::INFO, 
                        "Changed priority for PID " + to_string(pid) +
                        " from " + to_string(beforeChange.priority) +
                        " to " + to_string(afterChange.priority));
                } else {
                    cout << "[ERROR] Failed to change priority for process " << pid << ". Access denied or invalid priority level." << endl;
                    ImplementationLogger::log(ImplementationLogger::LOG_ERROR, "Failed to change priority for PID " + to_string(pid));
                }
                break;
            }
            case 4: {
                auto processes = listProcesses();
                ProcessLogger::update(processes);
                cout << "Process log updated (table format logged to file)." << endl;
                break;
            }
            case 5: {
                ImplementationLogger::log(ImplementationLogger::INFO, "Exiting SPM.");
                goto exit_loop;
            }
            default: {
                cout << "Invalid choice! Try again." << endl;
                ImplementationLogger::log(ImplementationLogger::DEBUG, "User entered an invalid menu choice.");
                break;
            }
        }
    }
exit_loop:
    ImplementationLogger::close();
    ProcessLogger::close();
    return 0;
}
