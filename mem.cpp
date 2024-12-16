#include "header.h"

void getMemoryInfo(struct sysinfo &memInfo) {
    sysinfo(&memInfo);
}

void getDiskUsage(struct statvfs &diskInfo) {
    statvfs("/", &diskInfo);
}

void getProcessInfo(vector<Proc> &processes) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("/proc")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (isdigit(ent->d_name[0])) {
                int pid = atoi(ent->d_name);
                string path = string("/proc/") + ent->d_name + "/stat";
                ifstream statFile(path);
                if (statFile.is_open()) {
                    Proc proc;
                    statFile >> proc.pid >> proc.name >> proc.state >> proc.utime >> proc.stime >> proc.vsize >> proc.rss;
                    processes.push_back(proc);
                    statFile.close();
                }
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
}

void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position) {
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    struct sysinfo memInfo;
    getMemoryInfo(memInfo);

    struct statvfs diskInfo;
    getDiskUsage(diskInfo);

    long totalDisk = diskInfo.f_blocks * diskInfo.f_frsize;
    long freeDisk = diskInfo.f_bfree * diskInfo.f_frsize;
    long usedDisk = totalDisk - freeDisk;

    ImGui::Text("Total RAM: %ld MB", memInfo.totalram / 1024 / 1024);
    ImGui::Text("Free RAM: %ld MB", memInfo.freeram / 1024 / 1024);
    ImGui::Text("Used RAM: %ld MB", (memInfo.totalram - memInfo.freeram) / 1024 / 1024);
    ImGui::ProgressBar((float)(memInfo.totalram - memInfo.freeram) / memInfo.totalram, ImVec2(0.0f, 0.0f));
    ImGui::Separator();

    ImGui::Text("Total Swap: %ld MB", memInfo.totalswap / 1024 / 1024);
    ImGui::Text("Free Swap: %ld MB", memInfo.freeswap / 1024 / 1024);
    ImGui::Text("Used Swap: %ld MB", (memInfo.totalswap - memInfo.freeswap) / 1024 / 1024);
    ImGui::ProgressBar((float)(memInfo.totalswap - memInfo.freeswap) / memInfo.totalswap, ImVec2(0.0f, 0.0f));
    ImGui::Separator();

    ImGui::Text("Total Disk: %ld MB", totalDisk / 1024 / 1024);
    ImGui::Text("Free Disk: %ld MB", freeDisk / 1024 / 1024);
    ImGui::Text("Used Disk: %ld MB", usedDisk / 1024 / 1024);
    ImGui::ProgressBar((float)usedDisk / totalDisk, ImVec2(0.0f, 0.0f));
    ImGui::Separator();

    static char filter[64] = "";
    ImGui::InputText("Filter", filter, IM_ARRAYSIZE(filter));

    vector<Proc> processes;
    getProcessInfo(processes);

    if (ImGui::BeginTabBar("Processes")) {
        if (ImGui::BeginTabItem("Processes")) {
            if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("PID");
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("State");
                ImGui::TableSetupColumn("CPU Usage");
                ImGui::TableSetupColumn("Memory Usage");
                ImGui::TableHeadersRow();

                static ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
                static bool selected[1024] = { false };

                for (size_t i = 0; i < processes.size(); i++) {
                    const auto &proc = processes[i];
                    if (strstr(proc.name.c_str(), filter) == NULL) continue;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Selectable(std::to_string(proc.pid).c_str(), &selected[i], selectable_flags);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", proc.name.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", proc.state);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.2f%%", (float)proc.utime / sysconf(_SC_CLK_TCK));
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.2f%%", (float)proc.rss * 100 / memInfo.totalram);
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}