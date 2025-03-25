#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "common.h"

struct options {
    char *host;
    uint16_t port;
    uint32_t runfor;
    bool verbose;
};

void print_usage(const char *argv0) {
    fprintf(stderr, "Usage: %s --host <string> --port <int> --runfor <int> [--verbose]\n", argv0);
}

int main(int argc, char **argv) {
    struct options opts = {0};
    opts.verbose = false;

    static struct option long_options[] = {
        {"host",    required_argument, 0, 'h'},
        {"port",    required_argument, 0, 'p'},
        {"runfor",  required_argument, 0, 'r'},
        {"verbose", no_argument,       0, 'v'},
        {NULL,      0,                 0, 0,}
    };

    int opt = 0;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "h:p:r:v", long_options, &option_index)) != -1) {
        switch (opt)
        {
        case 'h':
            opts.host = optarg;
            break;
        case 'p':
            opts.port = atoi(optarg);
            break;
        case 'r':
            opts.runfor = atoi(optarg);
            break;
        case 'v':
            opts.verbose = true;
            break;
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (opts.host == NULL || opts.port == 0 || opts.runfor == 0) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(opts.port);
    
    if (inet_pton(AF_INET, opts.host, &target.sin_addr) <= 0) {
        error_exit("Invalid target address");
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        error_exit("Unable to create socket");
    }

    //if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &(int) {1}, sizeof(int)) < 0) {
    //    close(sock);
    //    error_exit("Unable to set socket option");
    //}

    if (connect(sock, (struct sockaddr *) &target, sizeof(target)) < 0) {
        close(sock);
        error_exit("Unable to connect to server");
    }

    uint64_t T1, T2, T3, T4, t0;
    int64_t offset;
    T1 = get_time_ns();
    if (send(sock, &T1, sizeof(T1), 0) < 0) {
        close(sock);
        error_exit("Time sync: send failed");
    }
    if (recv(sock, &T2, sizeof(T2), 0) <= 0) {
        close(sock);
        error_exit("Time sync: recv failed");
    }
    T3 = get_time_ns();
    if (send(sock, &T3, sizeof(T3), 0) < 0) {
        close(sock);
        error_exit("Time sync: send failed");
    }
    if (recv(sock, &T4, sizeof(T4), 0) <= 0) {
        close(sock);
        error_exit("Time sync: recv failed");
    }

    offset = ((T2 - T1) + (T3 - T4)) / 2;
    printf("E2E Offset is %ldns\n", offset);

    if (offset > 0) {
        t0 = get_time_ns() - offset;
    } else {
        t0 = get_time_ns() + offset;
    }

    uint64_t runto = get_time_s() + opts.runfor;
    speedtest_packet_t packet;
    struct tcp_info info;
    uint64_t total_send = 0;
    socklen_t optlen;
    for (;;) {
        if (get_time_s() >= runto) break;

        packet.header.send_time_ns = get_time_ns() - t0;
        packet.header.recv_time_ns = 0;

        optlen = sizeof(info);
        if (getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, &optlen) < 0) {
            close(sock);
            error_exit("getsockopt RTT failed");
        }
        
        packet.header.rtt = info.tcpi_rtt;
        packet.header.cwnd = info.tcpi_snd_cwnd;
        packet.header.acknowledged_bytes = total_send - info.tcpi_unacked;
        memset(packet.payload, 0, sizeof(packet.payload));

        if (send(sock, &packet, sizeof(packet), 0) < 0) {
            close(sock);
            error_exit("Send failed");
        }
        total_send += PACKET_SIZE;
    }

    close(sock);
    return 0;
}
