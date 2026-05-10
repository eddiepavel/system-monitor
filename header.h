// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
// lib to read from file
#include <fstream>
#include <sstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
};

struct RX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long frame;
    long long compressed;
    long long multicast;
};

struct TX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long colls;
    long long carrier;
    long long compressed;
};

// system stats
string CPUinfo();
const char *getOsName();
string getLoggedInUser();
string getHostname();
void getProcessStats(int &running, int &sleeping, int &uninterruptible, int &zombie, int &tracedStopped, int &interruptible);
int getTotalTasks();
float getCPUUsage();
int getFanSpeed();
float getTemperature();
void systemWindow(const char *id, ImVec2 size, ImVec2 position);

// memory and processes
void getMemoryInfo(struct sysinfo &memInfo);
void getDiskUsage(struct statvfs &diskInfo);
void getProcessInfo(vector<Proc> &processes);
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position);

// network
void getNetworkInfo(Networks &networks);
void getNetworkStats(const char *interface, RX &rx, TX &tx);
void displayUsage(const char *label, long long bytes);
void networkWindow(const char *id, ImVec2 size, ImVec2 position);

#endif