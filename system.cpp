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
    // Initialize all counters to 0
    running = 0;
    sleeping = 0;
    uninterruptible = 0;
    zombie = 0;
    tracedStopped = 0;
    interruptible = 0;

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
                    string line;
                    getline(statFile, line);

                    // Find the last ')' to properly parse the state field
                    size_t lastParen = line.rfind(')');
                    if (lastParen != string::npos && lastParen + 2 < line.length())
                    {
                        char state = line[lastParen + 2];

                        if (state == 'R')
                            running++;
                        else if (state == 'S')
                            sleeping++;
                        else if (state == 'D')
                            uninterruptible++;
                        else if (state == 'Z')
                            zombie++;
                        else if (state == 'T')
                            tracedStopped++;
                        else if (state == 't')
                            tracedStopped++;
                        else if (state == 'I')
                            interruptible++;
                    }
                    statFile.close();
                }
            }
        }
        closedir(dir);
    }
}

// get total number of tasks/processes
int getTotalTasks()
{
    int running, sleeping, uninterruptible, zombie, tracedStopped, interruptible;
    getProcessStats(running, sleeping, uninterruptible, zombie, tracedStopped, interruptible);
    return running + sleeping + uninterruptible + zombie + tracedStopped + interruptible;
}

// get CPU usage
float getCPUUsage()
{
    static long long lastTotal = 0, lastIdle = 0;
    
    ifstream file("/proc/stat");
    string cpu;
    long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    
    file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;
    file.close();

    long long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long long idleTime = idle + iowait;

    if (lastTotal == 0)
    {
        lastTotal = total;
        lastIdle = idleTime;
        return 0;
    }

    long long totalDiff = total - lastTotal;
    long long idleDiff = idleTime - lastIdle;

    float percent = 100.0 * (totalDiff - idleDiff) / (float)totalDiff;

    lastTotal = total;
    lastIdle = idleTime;

    return percent;
}

// get fan speed
int getFanSpeed()
{
    ifstream file("/sys/class/hwmon/hwmon0/fan1_input");
    if (!file.is_open())
        return 0;
    int speed;
    file >> speed;
    file.close();
    return speed;
}

// get fan level
int getFanLevel()
{
    ifstream file("/sys/class/hwmon/hwmon0/pwm1");
    if (!file.is_open())
        return 0;
    int level;
    file >> level;
    file.close();
    return level;
}

// get temperature
float getTemperature()
{
    ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (file.is_open())
    {
        float temp;
        file >> temp;
        file.close();
        return temp / 1000.0;
    }

    file.open("/proc/acpi/ibm/thermal");
    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            if (line.find("temperatures:") != string::npos)
            {
                size_t pos = line.find_last_of(' ');
                if (pos != string::npos)
                {
                    float temp = stof(line.substr(pos + 1));
                    file.close();
                    return temp;
                }
            }
        }
        file.close();
    }

    return 0.0;
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
    int total = running + sleeping + uninterruptible + zombie + tracedStopped + interruptible;
    ImGui::Text("Total Tasks: %d", total);
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
    static int frameCount = 0;

    if (ImGui::BeginTabBar("SystemTabs"))
    {
        if (ImGui::BeginTabItem("CPU"))
        {
            ImGui::Checkbox("Animate", &animate);
            ImGui::SliderFloat("FPS##CPU", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale##CPU", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;

            // Update based on FPS slider (assuming 60 FPS render target)
            int frames_per_update = (int)(60.0f / fps);
            if (frames_per_update < 1) frames_per_update = 1;

            if (animate && frameCount % frames_per_update == 0)
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
            ImGui::Checkbox("Animate##Fan", &animate);
            ImGui::SliderFloat("FPS##Fan", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale##Fan", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;

            int frames_per_update = (int)(60.0f / fps);
            if (frames_per_update < 1) frames_per_update = 1;

            if (animate && frameCount % frames_per_update == 0)
            {
                values[values_offset] = getFanSpeed();
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
            }

            char overlay[32];
            sprintf(overlay, "Fan Speed: %d RPM", (int)values[(values_offset - 1 + IM_ARRAYSIZE(values)) % IM_ARRAYSIZE(values)]);
            ImGui::Text("Fan Status: %s", values[values_offset] > 0 ? "Active" : "Inactive");
            ImGui::Text("Fan Level: %d", getFanLevel());
            ImGui::PlotLines("##Fan", values, IM_ARRAYSIZE(values), values_offset, overlay, 0.0f, yScale, ImVec2(0, 80));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Thermal"))
        {
            ImGui::Checkbox("Animate##Thermal", &animate);
            ImGui::SliderFloat("FPS##Thermal", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale##Thermal", &yScale, 0.1f, 10.0f);

            static float values[90] = {0};
            static int values_offset = 0;

            int frames_per_update = (int)(60.0f / fps);
            if (frames_per_update < 1) frames_per_update = 1;

            if (animate && frameCount % frames_per_update == 0)
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

    frameCount++;

    ImGui::End();
}