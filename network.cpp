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

void displayUsage(const char *label, long bytes) {
    float value = bytes / (1024.0f * 1024.0f * 1024.0f); // Convert to GB
    if (value < 0.01f) {
        value = bytes / (1024.0f * 1024.0f); // Convert to MB
        ImGui::Text("%s: %.2f MB", label, value);
    } else {
        ImGui::Text("%s: %.2f GB", label, value);
    }
    ImGui::ProgressBar(value / 2.0f, ImVec2(0.0f, 0.0f)); // 2GB max
}

void networkWindow(const char *id, ImVec2 size, ImVec2 position) {
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    Networks networks;
    getNetworkInfo(networks);

    if (ImGui::BeginTabBar("NetworkTabs")) {
        if (ImGui::BeginTabItem("RX")) {
            for (const auto &ip4 : networks.ip4s) {
                RX rx;
                TX tx;
                getNetworkStats(ip4.name, rx, tx);
                ImGui::Text("Interface: %s", ip4.name);
                displayUsage("Bytes", rx.bytes);
                displayUsage("Packets", rx.packets);
                displayUsage("Errors", rx.errs);
                displayUsage("Dropped", rx.drop);
                displayUsage("FIFO", rx.fifo);
                displayUsage("Frame", rx.frame);
                displayUsage("Compressed", rx.compressed);
                displayUsage("Multicast", rx.multicast);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TX")) {
            for (const auto &ip4 : networks.ip4s) {
                TX tx;
                RX rx;
                getNetworkStats(ip4.name, rx, tx);
                ImGui::Text("Interface: %s", ip4.name);
                displayUsage("Bytes", tx.bytes);
                displayUsage("Packets", tx.packets);
                displayUsage("Errors", tx.errs);
                displayUsage("Dropped", tx.drop);
                displayUsage("FIFO", tx.fifo);
                displayUsage("Collisions", tx.colls);
                displayUsage("Carrier", tx.carrier);
                displayUsage("Compressed", tx.compressed);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}