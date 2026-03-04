#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include "../include/ProcessMonitor.h"
#include "../include/ProcessControl.h"
#include "../include/Logger.h"
#include "../include/WindowManager.h"
#include "imgui.h"

using namespace std;

// Global snapshot manager.
ProcessSnapshot g_Snapshot;
atomic<bool> g_Running(true);

// Background thread for automatic refresh.
void backgroundRefresh() {
    while (g_Running) {
        g_Snapshot.refresh();
        this_thread::sleep_for(chrono::seconds(2));
    }
}

int main() {
    // Initialize loggers.
    ImplementationLogger::init("logs/implementation_log.txt");
    ProcessLogger::init("logs/process_log.txt");
    ImplementationLogger::log(ImplementationLogger::INFO, "SPM GUI started.");

    // Initial refresh and start background thread.
    g_Snapshot.refresh();
    thread refreshThread(backgroundRefresh);

    WindowManager windowManager;
    if (!windowManager.init(1280, 720, "Smart Process Manager")) {
        g_Running = false;
        refreshThread.join();
        return 1;
    }

    static char searchFilter[128] = "";
    static int selectedPid = -1;

    // Main loop
    while (!windowManager.shouldClose()) {
        windowManager.startFrame();

        // UI Code
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Dashboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Columns(2, "MainColumns", true);
        ImGui::SetColumnWidth(0, ImGui::GetIO().DisplaySize.x * 0.75f);

        // --- LEFT COLUMN: Process List ---
        ImGui::Text("Smart Process Manager (SPM)");
        ImGui::SameLine();
        if (g_Snapshot.isUpdating()) {
            ImGui::TextDisabled("(Refreshing...)");
        } else {
            ImGui::TextDisabled("(Auto-refresh: 2s)");
        }
        
        ImGui::InputText("Search Process", searchFilter, IM_ARRAYSIZE(searchFilter));
        ImGui::Separator();

        auto processes = g_Snapshot.getProcesses();
        string filterStr = string(searchFilter);
        transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        if (ImGui::BeginTable("ProcessTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();

            for (const auto& proc : processes) {
                string nameLower = proc.name;
                transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

                if (!filterStr.empty() && nameLower.find(filterStr) == string::npos)
                    continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                
                bool isSelected = (selectedPid == proc.pid);
                if (ImGui::Selectable(to_string(proc.pid).c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                    selectedPid = proc.pid;
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", proc.name.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", proc.status.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", proc.priority);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.2f", proc.memoryMB);
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", proc.owner.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::NextColumn();

        // --- RIGHT COLUMN: Actions ---
        ImGui::Text("Actions");
        ImGui::Separator();

        if (selectedPid != -1) {
            ImGui::Text("Selected PID: %d", selectedPid);
            
            if (ImGui::Button("Terminate Process", ImVec2(-FLT_MIN, 0))) {
                if (killProcess(selectedPid)) {
                    ImplementationLogger::log(ImplementationLogger::INFO, "GUI: Terminated PID " + to_string(selectedPid));
                    selectedPid = -1;
                    g_Snapshot.refresh();
                }
            }

            ImGui::Separator();
            ImGui::Text("Change Priority:");
            static int priorityLevel = 3;
            ImGui::Combo("Level", &priorityLevel, "Idle\0Below Normal\0Normal\0Above Normal\0High\0");
            
            if (ImGui::Button("Apply Priority", ImVec2(-FLT_MIN, 0))) {
                if (changeProcessPriority(selectedPid, priorityLevel + 1)) {
                    ImplementationLogger::log(ImplementationLogger::INFO, "GUI: Changed Priority for PID " + to_string(selectedPid));
                }
            }
        } else {
            ImGui::TextDisabled("Select a process to perform actions.");
        }

        ImGui::End();

        windowManager.endFrame();
    }

    g_Running = false;
    refreshThread.join();
    ImplementationLogger::log(ImplementationLogger::INFO, "Exiting SPM GUI.");
    return 0;
}
