#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// check if the device is online
// return 1 if online, 0 if offline, -1 if error
int check_online_status() {
    struct ifaddrs *ifaddr, *ifa;
    int online = 0;

    // Get list of all network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        return -1;
    }

    // Iterate through the list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        // Skip loopback interface
        if (strcmp(ifa->ifa_name, "lo") == 0) continue;

        // Check only IPv4 addresses
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);

            // If we find a valid non-zero IP address, device is online
            if (strcmp(ip, "0.0.0.0") != 0) {
                //printf("Online - Interface: %s, IP: %s\n", ifa->ifa_name, ip);
                online = 1;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return online;
}