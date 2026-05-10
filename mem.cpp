#include "header.h"

// Static variables to track previous CPU times for each process
static map<int, pair<long long, long long>> prevProcCPU;  // pid -> (prev_utime+stime, prev_total_time)
static long long prevTotalCPU = 0;

void getMemoryInfo(struct sysinfo &memInfo) {
    sysinfo(&memInfo);

    // Read /proc/meminfo for more accurate memory info including cached memory
    ifstream memfile("/proc/meminfo");
    if (memfile.is_open()) {
        string line;
        long long cached = 0, buffers = 0;
        while (getline(memfile, line)) {
            if (line.find("Cached:") == 0) {
                sscanf(line.c_str(), "Cached: %lld", &cached);
                cached *= 1024;
            } else if (line.find("Buffers:") == 0) {
                sscanf(line.c_str(), "Buffers: %lld", &buffers);
                buffers *= 1024;
            }
        }
        memfile.close();

        // Adjust memInfo to account for cache (subtract cache from used)
        // This makes the display match 'free' command more accurately
        memInfo.bufferram = cached + buffers;
    }
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
                    string line;
                    getline(statFile, line);
                    
                    Proc proc;
                    proc.pid = pid;
                    
                    // Parse process name (it's between parentheses)
                    size_t firstParen = line.find('(');
                    size_t lastParen = line.rfind(')');
                    if (firstParen != string::npos && lastParen != string::npos) {
                        proc.name = line.substr(firstParen + 1, lastParen - firstParen - 1);
                        
                        // Parse fields after the closing parenthesis
                        string rest = line.substr(lastParen + 2);
                        istringstream iss(rest);
                        
                        // Field order after name: state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime...
                        string dummy;
                        iss >> proc.state;  // field 3
                        
                        // Skip fields 4-13 to get to utime (field 14) and stime (field 15)
                        for (int i = 0; i < 10; i++) iss >> dummy;
                        
                        iss >> proc.utime >> proc.stime;
                        
                        // Skip cutime and cstime (fields 16-17) to get to vsize (field 23) and rss (field 24)
                        for (int i = 0; i < 5; i++) iss >> dummy;
                        
                        iss >> proc.vsize >> proc.rss;
                    }
                    
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
    long usedRam = memInfo.totalram - memInfo.freeram - memInfo.bufferram;
    if (usedRam < 0) usedRam = 0;
    ImGui::Text("Used RAM: %ld MB", usedRam / 1024 / 1024);
    ImGui::ProgressBar((float)usedRam / memInfo.totalram, ImVec2(0.0f, 0.0f));
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
    
    // Get current total CPU time
    long long totalCPU = 0;
    ifstream cpuFile("/proc/stat");
    if (cpuFile.is_open()) {
        string cpu;
        long long user, nice, system, idle, iowait, irq, softirq, steal;
        cpuFile >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        totalCPU = user + nice + system + idle + iowait + irq + softirq + steal;
        cpuFile.close();
    }

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
                    if (strlen(filter) > 0 && strstr(proc.name.c_str(), filter) == NULL) continue;

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Selectable(std::to_string(proc.pid).c_str(), &selected[i], selectable_flags);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", proc.name.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%c", proc.state);
                    ImGui::TableSetColumnIndex(3);
                    
                    // Calculate CPU usage percentage
                    float cpuPercent = 0.0f;
                    long long procTime = proc.utime + proc.stime;
                    
                    if (prevProcCPU.find(proc.pid) != prevProcCPU.end() && prevTotalCPU > 0) {
                        long long prevProcTime = prevProcCPU[proc.pid].first;
                        long long procDelta = procTime - prevProcTime;
                        long long totalDelta = totalCPU - prevTotalCPU;
                        
                        if (totalDelta > 0) {
                            cpuPercent = 100.0f * procDelta / totalDelta;
                        }
                    }
                    
                    prevProcCPU[proc.pid] = make_pair(procTime, totalCPU);
                    
                    ImGui::Text("%.2f%%", cpuPercent);
                    
                    ImGui::TableSetColumnIndex(4);
                    // RSS is in pages, convert to percentage of total RAM
                    long pageSize = sysconf(_SC_PAGESIZE);
                    float memPercent = (float)(proc.rss * pageSize) * 100.0f / memInfo.totalram;
                    ImGui::Text("%.2f%%", memPercent);
                }
                
                prevTotalCPU = totalCPU;
                
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}