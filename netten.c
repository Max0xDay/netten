#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/wireless.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>

#define MAX_INTERFACES 64
#define BUFFER_SIZE 4096
#define PROC_NET_DEV "/proc/net/dev"
#define SYS_CLASS_NET "/sys/class/net"

#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 1
#endif

struct interface_stats {
    char name[IFNAMSIZ];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_errors;
    unsigned long long tx_errors;
};

struct port_info {
    int port;
    char protocol[8];
    char state[16];
    struct in_addr local_addr;
    struct in_addr remote_addr;
};

void show_help() {
    printf("netten - Network Information Tool\n\n");
    printf("Usage: netten [command]\n\n");
    printf("Commands:\n");
    printf("  help          Show this help message\n");
    printf("  interfaces    List all network interfaces with details\n");
    printf("  ports         Show all listening ports\n");
    printf("  traffic       Display traffic statistics for all interfaces\n");
    printf("  ip            Show IP addresses for all interfaces\n");
    printf("  mac           Show MAC addresses for all interfaces\n");
    printf("  drivers       Show network driver information\n");
    printf("  speed         Show interface link speeds\n");
    printf("  stats         Show detailed network statistics\n");
    printf("  connections   Show active network connections\n");
    printf("  route         Show routing table\n");
}

void get_mac_address(const char *iface, char *mac) {
    int fd;
    struct ifreq ifr;
    unsigned char *hw_addr;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        strcpy(mac, "00:00:00:00:00:00");
        return;
    }
    
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        hw_addr = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
                hw_addr[0], hw_addr[1], hw_addr[2],
                hw_addr[3], hw_addr[4], hw_addr[5]);
    } else {
        strcpy(mac, "00:00:00:00:00:00");
    }
    
    close(fd);
}

void show_interfaces() {
    struct ifaddrs *ifaddr, *ifa;
    char mac[18];
    int family;
    
    printf("Network Interfaces:\n");
    printf("%-15s %-20s %-18s %-10s\n", "Interface", "IP Address", "MAC Address", "Status");
    printf("----------------------------------------------------------------------\n");
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        family = ifa->ifa_addr->sa_family;
        
        if (family == AF_INET) {
            char host[NI_MAXHOST];
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            
            if (s == 0) {
                get_mac_address(ifa->ifa_name, mac);
                printf("%-15s %-20s %-18s %-10s\n",
                       ifa->ifa_name, host, mac,
                       (ifa->ifa_flags & IFF_UP) ? "UP" : "DOWN");
            }
        }
    }
    
    freeifaddrs(ifaddr);
}

void parse_proc_net_file(const char *filename, const char *protocol) {
    FILE *fp = fopen(filename, "r");
    char line[512];
    
    if (!fp) return;
    
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        unsigned long local_addr, local_port;
        unsigned long remote_addr, remote_port;
        int state;
        
        if (sscanf(line, "%*d: %lX:%lX %lX:%lX %X",
                   &local_addr, &local_port,
                   &remote_addr, &remote_port, &state) == 5) {
            
            if (state == 0x0A || state == 0x01) {
                struct in_addr addr;
                addr.s_addr = local_addr;
                printf("%-6s %-20s:%-5lu ", protocol,
                       inet_ntoa(addr), local_port);
                
                if (state == 0x0A) {
                    printf("LISTEN\n");
                } else {
                    addr.s_addr = remote_addr;
                    printf("ESTABLISHED -> %s:%lu\n",
                           inet_ntoa(addr), remote_port);
                }
            }
        }
    }
    
    fclose(fp);
}

void show_ports() {
    printf("Active Ports:\n");
    printf("%-6s %-26s %s\n", "Proto", "Local Address", "State");
    printf("-----------------------------------------------\n");
    
    parse_proc_net_file("/proc/net/tcp", "tcp");
    parse_proc_net_file("/proc/net/tcp6", "tcp6");
    parse_proc_net_file("/proc/net/udp", "udp");
    parse_proc_net_file("/proc/net/udp6", "udp6");
}

void read_interface_stats(struct interface_stats *stats, int *count) {
    FILE *fp = fopen(PROC_NET_DEV, "r");
    char line[512];
    *count = 0;
    
    if (!fp) return;
    
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp) && *count < MAX_INTERFACES) {
        char *p = strchr(line, ':');
        if (p) {
            *p = '\0';
            char *name = line;
            while (*name == ' ') name++;
            
            strncpy(stats[*count].name, name, IFNAMSIZ - 1);
            
            sscanf(p + 1, "%llu %llu %*u %*u %*u %*u %*u %*u %llu %llu %llu %llu",
                   &stats[*count].rx_bytes, &stats[*count].rx_packets,
                   &stats[*count].tx_bytes, &stats[*count].tx_packets,
                   &stats[*count].rx_errors, &stats[*count].tx_errors);
            
            (*count)++;
        }
    }
    
    fclose(fp);
}

void show_traffic() {
    struct interface_stats stats[MAX_INTERFACES];
    int count;
    
    printf("Network Traffic Statistics:\n");
    printf("%-15s %15s %15s %15s %15s\n",
           "Interface", "RX Bytes", "TX Bytes", "RX Packets", "TX Packets");
    printf("--------------------------------------------------------------------------------\n");
    
    read_interface_stats(stats, &count);
    
    for (int i = 0; i < count; i++) {
        printf("%-15s %15llu %15llu %15llu %15llu\n",
               stats[i].name,
               stats[i].rx_bytes, stats[i].tx_bytes,
               stats[i].rx_packets, stats[i].tx_packets);
    }
}

void show_ip_addresses() {
    struct ifaddrs *ifaddr, *ifa;
    
    printf("IP Addresses:\n");
    printf("%-15s %-40s %s\n", "Interface", "IPv4 Address", "IPv6 Address");
    printf("----------------------------------------------------------------------\n");
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        char host[NI_MAXHOST];
        int family = ifa->ifa_addr->sa_family;
        
        if (family == AF_INET || family == AF_INET6) {
            int s = getnameinfo(ifa->ifa_addr,
                               (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                    sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            
            if (s == 0) {
                printf("%-15s %-40s\n", ifa->ifa_name,
                       (family == AF_INET) ? host : "");
                if (family == AF_INET6) {
                    printf("%-15s %-40s %s\n", "", "", host);
                }
            }
        }
    }
    
    freeifaddrs(ifaddr);
}

void show_mac_addresses() {
    struct if_nameindex *if_nidxs, *intf;
    char mac[18];
    
    printf("MAC Addresses:\n");
    printf("%-15s %s\n", "Interface", "MAC Address");
    printf("----------------------------------\n");
    
    if_nidxs = if_nameindex();
    if (if_nidxs != NULL) {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++) {
            get_mac_address(intf->if_name, mac);
            if (strcmp(mac, "00:00:00:00:00:00") != 0) {
                printf("%-15s %s\n", intf->if_name, mac);
            }
        }
        if_freenameindex(if_nidxs);
    }
}

void get_driver_info(const char *iface) {
    int fd;
    struct ifreq ifr;
    struct ethtool_drvinfo drvinfo;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return;
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    
    drvinfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = (char *)&drvinfo;
    
    if (ioctl(fd, SIOCETHTOOL, &ifr) == 0) {
        printf("%-15s %-20s %-15s %s\n",
               iface, drvinfo.driver, drvinfo.version, drvinfo.bus_info);
    }
    
    close(fd);
}

void show_drivers() {
    struct if_nameindex *if_nidxs, *intf;
    
    printf("Network Driver Information:\n");
    printf("%-15s %-20s %-15s %s\n", "Interface", "Driver", "Version", "Bus Info");
    printf("----------------------------------------------------------------------\n");
    
    if_nidxs = if_nameindex();
    if (if_nidxs != NULL) {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++) {
            get_driver_info(intf->if_name);
        }
        if_freenameindex(if_nidxs);
    }
}

void get_link_speed(const char *iface) {
    int fd;
    struct ifreq ifr;
    struct ethtool_cmd edata;
    
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return;
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    
    edata.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (char *)&edata;
    
    if (ioctl(fd, SIOCETHTOOL, &ifr) == 0) {
        printf("%-15s ", iface);
        
        if (edata.speed == (unsigned short)(-1)) {
            printf("Unknown\n");
        } else {
            printf("%u Mbps\n", edata.speed);
        }
    }
    
    close(fd);
}

void show_speeds() {
    struct if_nameindex *if_nidxs, *intf;
    
    printf("Interface Link Speeds:\n");
    printf("%-15s %s\n", "Interface", "Speed");
    printf("----------------------------------\n");
    
    if_nidxs = if_nameindex();
    if (if_nidxs != NULL) {
        for (intf = if_nidxs; intf->if_index != 0 || intf->if_name != NULL; intf++) {
            get_link_speed(intf->if_name);
        }
        if_freenameindex(if_nidxs);
    }
}

void show_detailed_stats() {
    struct interface_stats stats[MAX_INTERFACES];
    int count;
    
    printf("Detailed Network Statistics:\n");
    printf("%-15s %10s %10s %10s %10s %10s %10s\n",
           "Interface", "RX Bytes", "TX Bytes", "RX Packets", "TX Packets", "RX Errors", "TX Errors");
    printf("----------------------------------------------------------------------------------------\n");
    
    read_interface_stats(stats, &count);
    
    for (int i = 0; i < count; i++) {
        printf("%-15s %10llu %10llu %10llu %10llu %10llu %10llu\n",
               stats[i].name,
               stats[i].rx_bytes, stats[i].tx_bytes,
               stats[i].rx_packets, stats[i].tx_packets,
               stats[i].rx_errors, stats[i].tx_errors);
    }
}

void show_connections() {
    printf("Active Network Connections:\n");
    system("ss -tunap 2>/dev/null | grep -E 'ESTAB|LISTEN' | head -20");
}

void show_routing_table() {
    FILE *fp = fopen("/proc/net/route", "r");
    char line[512];
    
    if (!fp) {
        perror("Cannot open routing table");
        return;
    }
    
    printf("Routing Table:\n");
    printf("%-15s %-15s %-15s %-10s %s\n",
           "Destination", "Gateway", "Netmask", "Flags", "Interface");
    printf("----------------------------------------------------------------------\n");
    
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp)) {
        char iface[16];
        unsigned long dest, gateway, mask;
        int flags, metric;
        
        if (sscanf(line, "%s %lX %lX %X %*d %*d %d %lX",
                   iface, &dest, &gateway, &flags, &metric, &mask) == 6) {
            
            struct in_addr addr;
            
            addr.s_addr = dest;
            printf("%-15s ", inet_ntoa(addr));
            
            addr.s_addr = gateway;
            printf("%-15s ", gateway ? inet_ntoa(addr) : "*");
            
            addr.s_addr = mask;
            printf("%-15s ", inet_ntoa(addr));
            
            printf("%-10X %s\n", flags, iface);
        }
    }
    
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_help();
        return 0;
    }
    
    if (strcmp(argv[1], "help") == 0) {
        show_help();
    } else if (strcmp(argv[1], "interfaces") == 0) {
        show_interfaces();
    } else if (strcmp(argv[1], "ports") == 0) {
        show_ports();
    } else if (strcmp(argv[1], "traffic") == 0) {
        show_traffic();
    } else if (strcmp(argv[1], "ip") == 0) {
        show_ip_addresses();
    } else if (strcmp(argv[1], "mac") == 0) {
        show_mac_addresses();
    } else if (strcmp(argv[1], "drivers") == 0) {
        show_drivers();
    } else if (strcmp(argv[1], "speed") == 0) {
        show_speeds();
    } else if (strcmp(argv[1], "stats") == 0) {
        show_detailed_stats();
    } else if (strcmp(argv[1], "connections") == 0) {
        show_connections();
    } else if (strcmp(argv[1], "route") == 0) {
        show_routing_table();
    } else {
        printf("Unknown command: %s\n", argv[1]);
        printf("Use 'netten help' for available commands.\n");
        return 1;
    }
    
    return 0;
}
