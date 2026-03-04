#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "../include/ProcessMonitor.h"
#include "../include/ProcessControl.h"
#include "../include/Logger.h"
#include "../include/WindowManager.h"
#include "imgui.h"

using namespace std;

// Global snapshot manager.
ProcessSnapshot g_Snapshot;

int main() {
    // Initialize loggers.
    ImplementationLogger::init("logs/implementation_log.txt");
    ProcessLogger::init("logs/process_log.txt");
    ImplementationLogger::log(ImplementationLogger::INFO, "SPM GUI started.");

    // Initial refresh to populate the snapshot.
    g_Snapshot.refresh();

    WindowManager windowManager;
    if (!windowManager.init(1280, 720, "Smart Process Manager")) {
        return 1;
    }

    // Main loop
    while (!windowManager.shouldClose()) {
        windowManager.startFrame();

        // UI Code
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Dashboard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Smart Process Manager (SPM)");
        ImGui::Separator();

        if (ImGui::Button("Refresh Processes")) {
            // Start refresh in a separate thread to avoid blocking UI
            std::thread([]() {
                g_Snapshot.refresh();
            }).detach();
        }

        if (g_Snapshot.isUpdating()) {
            ImGui::SameLine();
            ImGui::Text("Refreshing...");
        }

        ImGui::Separator();

        // Process Table
        auto processes = g_Snapshot.getProcesses();
        if (ImGui::BeginTable("ProcessTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Priority", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Mem (MB)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableHeadersRow();

            for (const auto& proc : processes) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", proc.pid);
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

        ImGui::End();

        windowManager.endFrame();
    }

    ImplementationLogger::log(ImplementationLogger::INFO, "Exiting SPM GUI.");
    return 0;
}
