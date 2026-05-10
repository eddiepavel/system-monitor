#include "header.h"

void getNetworkInfo(Networks &networks) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            IP4 ip4;
            ip4.name = ifa->ifa_name;
            inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, ip4.addressBuffer, INET_ADDRSTRLEN);
            networks.ip4s.push_back(ip4);
        }
    }

    freeifaddrs(ifaddr);
}

void getNetworkStats(const char *interface, RX &rx, TX &tx) {
    string path = string("/sys/class/net/") + interface + "/statistics/";
    ifstream file;

    file.open(path + "rx_bytes");
    file >> rx.bytes;
    file.close();

    file.open(path + "rx_packets");
    file >> rx.packets;
    file.close();

    file.open(path + "rx_errors");
    file >> rx.errs;
    file.close();

    file.open(path + "rx_dropped");
    file >> rx.drop;
    file.close();

    file.open(path + "rx_fifo_errors");
    file >> rx.fifo;
    file.close();

    file.open(path + "rx_frame_errors");
    file >> rx.frame;
    file.close();

    file.open(path + "rx_compressed");
    file >> rx.compressed;
    file.close();

    file.open(path + "rx_multicast");
    file >> rx.multicast;
    file.close();

    file.open(path + "tx_bytes");
    file >> tx.bytes;
    file.close();

    file.open(path + "tx_packets");
    file >> tx.packets;
    file.close();

    file.open(path + "tx_errors");
    file >> tx.errs;
    file.close();

    file.open(path + "tx_dropped");
    file >> tx.drop;
    file.close();

    file.open(path + "tx_fifo_errors");
    file >> tx.fifo;
    file.close();

    file.open(path + "collisions");
    file >> tx.colls;
    file.close();

    file.open(path + "carrier_errors");
    file >> tx.carrier;
    file.close();

    file.open(path + "tx_compressed");
    file >> tx.compressed;
    file.close();
}

void displayUsage(const char *label, long long bytes) {
    float value;
    char unit[4];
    
    // Convert bytes to appropriate unit
    if (bytes >= 1024LL * 1024 * 1024) {
        value = bytes / (1024.0f * 1024.0f * 1024.0f);
        strcpy(unit, "GB");
    } else if (bytes >= 1024 * 1024) {
        value = bytes / (1024.0f * 1024.0f);
        strcpy(unit, "MB");
    } else if (bytes >= 1024) {
        value = bytes / 1024.0f;
        strcpy(unit, "KB");
    } else {
        value = (float)bytes;
        strcpy(unit, "B");
    }
    
    ImGui::Text("%s: %.2f %s", label, value, unit);
    
    // Progress bar from 0 to 2GB
    float progress = bytes / (2.0f * 1024 * 1024 * 1024);
    if (progress > 1.0f) progress = 1.0f;
    ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
}

void networkWindow(const char *id, ImVec2 size, ImVec2 position) {
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    Networks networks;
    getNetworkInfo(networks);

    ImGui::Text("Network Interfaces:");
    for (const auto &ip4 : networks.ip4s) {
        ImGui::Text("  %s: %s", ip4.name, ip4.addressBuffer);
    }
    ImGui::Separator();

    static bool animate = true;
    static float fps = 30.0f;
    static float yScale = 1.0f;
    static int frameCount = 0;

    // Track previous RX/TX bytes for delta calculation
    static map<string, long long> prevRxBytes;
    static map<string, long long> prevTxBytes;

    if (ImGui::BeginTabBar("NetworkTabs")) {
        if (ImGui::BeginTabItem("RX")) {
            ImGui::Checkbox("Animate##RX", &animate);
            ImGui::SliderFloat("FPS##RX", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale##RX", &yScale, 0.1f, 10.0f);

            int frames_per_update = (int)(60.0f / fps);
            if (frames_per_update < 1) frames_per_update = 1;

            if (ImGui::BeginTabBar("RXInterfaces")) {
                for (const auto &ip4 : networks.ip4s) {
                    if (ImGui::BeginTabItem(ip4.name)) {
                        RX rx;
                        TX tx;
                        getNetworkStats(ip4.name, rx, tx);

                        ImGui::Text("Receive Statistics for %s", ip4.name);
                        ImGui::Separator();

                        displayUsage("Bytes", rx.bytes);
                        ImGui::Text("Packets: %lld", rx.packets);
                        ImGui::Text("Errors: %lld", rx.errs);
                        ImGui::Text("Dropped: %lld", rx.drop);
                        ImGui::Text("FIFO: %lld", rx.fifo);
                        ImGui::Text("Frame: %lld", rx.frame);
                        ImGui::Text("Compressed: %lld", rx.compressed);
                        ImGui::Text("Multicast: %lld", rx.multicast);

                        ImGui::Separator();
                        ImGui::Text("RX Traffic Graph (Bytes/sec):");
                        static map<string, vector<long long>> rxHistory;
                        static map<string, int> rxOffset;

                        if (rxHistory.find(ip4.name) == rxHistory.end()) {
                            rxHistory[ip4.name] = vector<long long>(90, 0);
                            rxOffset[ip4.name] = 0;
                            prevRxBytes[ip4.name] = rx.bytes;
                        }

                        if (animate && frameCount % frames_per_update == 0) {
                            // Calculate delta (bytes transferred since last reading)
                            long long delta = rx.bytes - prevRxBytes[ip4.name];
                            if (delta < 0) delta = 0; // Handle counter reset

                            rxHistory[ip4.name][rxOffset[ip4.name]] = delta;
                            rxOffset[ip4.name] = (rxOffset[ip4.name] + 1) % 90;
                            prevRxBytes[ip4.name] = rx.bytes;
                        }

                        float *values = new float[90];
                        long long maxValue = 0;
                        for (int i = 0; i < 90; i++) {
                            values[i] = (float)rxHistory[ip4.name][i] / (1024.0f * 1024.0f);
                            if (rxHistory[ip4.name][i] > maxValue) maxValue = rxHistory[ip4.name][i];
                        }

                        char overlay[32];
                        long long currentDelta = rxHistory[ip4.name][(rxOffset[ip4.name] - 1 + 90) % 90];
                        if (currentDelta > 1024*1024*1024) {
                            sprintf(overlay, "RX: %.2f GB/s", (float)currentDelta / (1024.0f*1024.0f*1024.0f));
                        } else if (currentDelta > 1024*1024) {
                            sprintf(overlay, "RX: %.2f MB/s", (float)currentDelta / (1024.0f*1024.0f));
                        } else if (currentDelta > 1024) {
                            sprintf(overlay, "RX: %.2f KB/s", (float)currentDelta / 1024.0f);
                        } else {
                            sprintf(overlay, "RX: %lld B/s", currentDelta);
                        }
                        ImGui::PlotLines("##RXGraph", values, 90, rxOffset[ip4.name], overlay, 0.0f, yScale, ImVec2(0, 80));
                        delete[] values;

                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TX")) {
            ImGui::Checkbox("Animate##TX", &animate);
            ImGui::SliderFloat("FPS##TX", &fps, 1.0f, 60.0f);
            ImGui::SliderFloat("Y Scale##TX", &yScale, 0.1f, 10.0f);

            int frames_per_update = (int)(60.0f / fps);
            if (frames_per_update < 1) frames_per_update = 1;

            if (ImGui::BeginTabBar("TXInterfaces")) {
                for (const auto &ip4 : networks.ip4s) {
                    if (ImGui::BeginTabItem(ip4.name)) {
                        TX tx;
                        RX rx;
                        getNetworkStats(ip4.name, rx, tx);

                        ImGui::Text("Transmit Statistics for %s", ip4.name);
                        ImGui::Separator();

                        displayUsage("Bytes", tx.bytes);
                        ImGui::Text("Packets: %lld", tx.packets);
                        ImGui::Text("Errors: %lld", tx.errs);
                        ImGui::Text("Dropped: %lld", tx.drop);
                        ImGui::Text("FIFO: %lld", tx.fifo);
                        ImGui::Text("Collisions: %lld", tx.colls);
                        ImGui::Text("Carrier: %lld", tx.carrier);
                        ImGui::Text("Compressed: %lld", tx.compressed);

                        ImGui::Separator();
                        ImGui::Text("TX Traffic Graph (Bytes/sec):");
                        static map<string, vector<long long>> txHistory;
                        static map<string, int> txOffset;

                        if (txHistory.find(ip4.name) == txHistory.end()) {
                            txHistory[ip4.name] = vector<long long>(90, 0);
                            txOffset[ip4.name] = 0;
                            prevTxBytes[ip4.name] = tx.bytes;
                        }

                        if (animate && frameCount % frames_per_update == 0) {
                            // Calculate delta (bytes transferred since last reading)
                            long long delta = tx.bytes - prevTxBytes[ip4.name];
                            if (delta < 0) delta = 0; // Handle counter reset

                            txHistory[ip4.name][txOffset[ip4.name]] = delta;
                            txOffset[ip4.name] = (txOffset[ip4.name] + 1) % 90;
                            prevTxBytes[ip4.name] = tx.bytes;
                        }

                        float *values = new float[90];
                        long long maxValue = 0;
                        for (int i = 0; i < 90; i++) {
                            values[i] = (float)txHistory[ip4.name][i] / (1024.0f * 1024.0f);
                            if (txHistory[ip4.name][i] > maxValue) maxValue = txHistory[ip4.name][i];
                        }

                        char overlay[32];
                        long long currentDelta = txHistory[ip4.name][(txOffset[ip4.name] - 1 + 90) % 90];
                        if (currentDelta > 1024*1024*1024) {
                            sprintf(overlay, "TX: %.2f GB/s", (float)currentDelta / (1024.0f*1024.0f*1024.0f));
                        } else if (currentDelta > 1024*1024) {
                            sprintf(overlay, "TX: %.2f MB/s", (float)currentDelta / (1024.0f*1024.0f));
                        } else if (currentDelta > 1024) {
                            sprintf(overlay, "TX: %.2f KB/s", (float)currentDelta / 1024.0f);
                        } else {
                            sprintf(overlay, "TX: %lld B/s", currentDelta);
                        }
                        ImGui::PlotLines("##TXGraph", values, 90, txOffset[ip4.name], overlay, 0.0f, yScale, ImVec2(0, 80));
                        delete[] values;

                        ImGui::EndTabItem();
                    }
                }
                ImGui::EndTabBar();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    frameCount++;
    ImGui::End();
}