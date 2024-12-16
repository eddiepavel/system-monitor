#include "header.h"

// get cpu id and information, you can use `proc/cpuinfo`
string CPUinfo()
{
    char CPUBrandString[0x40];
    unsigned int CPUInfo[4] = {0, 0, 0, 0};

    __cpuid(0x80000000, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
    unsigned int nExIds = CPUInfo[0];

    memset(CPUBrandString, 0, sizeof(CPUBrandString));

    for (unsigned int i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(i, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);

        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    string str(CPUBrandString);
    return str;
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// get the logged-in user
string getLoggedInUser()
{
    char username[LOGIN_NAME_MAX];
    getlogin_r(username, sizeof(username));
    return string(username);
}

// get the hostname
string getHostname()
{
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname));
    return string(hostname);
}

// get process information
void getProcessStats(int &running, int &sleeping, int &uninterruptible, int &zombie, int &tracedStopped, int &interruptible)
{
    ifstream file("/proc/stat");
    string line;
    while (getline(file, line))
    {
        if (line.find("procs_running") != string::npos)
        {
            sscanf(line.c_str(), "procs_running %d", &running);
        }
        else if (line.find("procs_blocked") != string::npos)
        {
            sscanf(line.c_str(), "procs_blocked %d", &uninterruptible);
        }
    }
    file.close();

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir("/proc")) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (isdigit(ent->d_name[0]))
            {
                string path = string("/proc/") + ent->d_name + "/stat";
                ifstream statFile(path);
                if (statFile.is_open())
                {
                    string state;
                    statFile >> state >> state >> state;
                    if (state == "S")
                        sleeping++;
                    else if (state == "Z")
                        zombie++;
                    else if (state == "T")
                        tracedStopped++;
                    else if (state == "I")
                        interruptible++;
                    statFile.close();
                }
            }
        }
        closedir(dir);
    }
}

// get CPU usage
float getCPUUsage()
{
    static long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;
    long long totalUser, totalUserLow, totalSys, totalIdle, total;

    ifstream file("/proc/stat");
    file >> totalUser >> totalUserLow >> totalSys >> totalIdle;
    file.close();

    if (lastTotalUser == 0)
    {
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        return 0;
    }

    total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) + (totalSys - lastTotalSys);
    float percent = total;
    total += (totalIdle - lastTotalIdle);
    percent /= total;
    percent *= 100;

    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;

    return percent;
}

// get fan speed
int getFanSpeed()
{
    ifstream file("/sys/class/hwmon/hwmon0/fan1_input");
    int speed;
    file >> speed;
    file.close();
    return speed;
}

// get fan level
int getFanLevel()
{
    ifstream file("/sys/class/hwmon/hwmon0/pwm1");
    int level;
    file >> level;
    file.close();
    return level;
}

// get temperature
float getTemperature()
{
    ifstream file("/sys/class/thermal/thermal_zone0/temp");
    float temp;
    file >> temp;
    file.close();
    return temp / 1000.0;
}

void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    ImGui::Text("Operating System: %s", getOsName());
    ImGui::Text("Logged in User: %s", getLoggedInUser().c_str());
    ImGui::Text("Hostname: %s", getHostname().c_str());

    int running = 0, sleeping = 0, uninterruptible = 0, zombie = 0, tracedStopped = 0, interruptible = 0;
    getProcessStats(running, sleeping, uninterruptible, zombie, tracedStopped, interruptible);
    ImGui::Text("Running: %d", running);
    ImGui::Text("Sleeping: %d", sleeping);
    ImGui::Text("Uninterruptible: %d", uninterruptible);
    ImGui::Text("Zombie: %d", zombie);
    ImGui::Text("Traced/Stopped: %d", tracedStopped);
    ImGui::Text("Interruptible: %d", interruptible);

    ImGui::Text("CPU: %s", CPUinfo().c_str());

    static bool animate = true;
    static float fps = 30.0f;
    static float yScale = 1.0f;

    if (ImGui::BeginTabBar("SystemTabs"))
    {
        if (ImGui::BeginTabItem("CPU"))
        {
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;
            if (animate)
            {
                values[values_offset] = getCPUUsage();
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            }

            char overlay[32];
            sprintf(overlay, "CPU Usage: %.2f%%", values[(values_offset - 1 + IM_ARRAYSIZE(values)) % IM_ARRAYSIZE(values)]);
            ImGui::PlotLines("##CPU", values, IM_ARRAYSIZE(values), values_offset, overlay, 0.0f, yScale, ImVec2(0, 80));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Fan"))
        {
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;
            if (animate)
            {
                values[values_offset] = getFanSpeed();
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            }

            char overlay[32];
            sprintf(overlay, "Fan Speed: %d RPM", (int)values[(values_offset - 1 + IM_ARRAYSIZE(values)) % IM_ARRAYSIZE(values)]);
            ImGui::Text("Fan Status: %s", values[values_offset] > 0 ? "Active" : "Inactive");
            ImGui::Text("Fan Level: %d", getFanLevel()); // Add this line to display fan level
            ImGui::PlotLines("##Fan", values, IM_ARRAYSIZE(values), values_offset, overlay, 0.0f, yScale, ImVec2(0, 80));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Thermal"))
        {
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;
            if (animate)
            {
                values[values_offset] = getTemperature();
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            }

            char overlay[32];
            sprintf(overlay, "Temperature: %.2f°C", values[(values_offset - 1 + IM_ARRAYSIZE(values)) % IM_ARRAYSIZE(values)]);
            ImGui::PlotLines("##Thermal", values, IM_ARRAYSIZE(values), values_offset, overlay, 0.0f, yScale, ImVec2(0, 80));
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}