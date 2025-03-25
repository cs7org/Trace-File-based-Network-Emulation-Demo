#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#include "common.h"

#define RESULTS_INIT_SIZE 500000
#define RESULTS_INCREMENT 100000

struct options {
    uint16_t port;
    char *file;
    bool verbose;
};

void print_usage(const char *argv0) {
    fprintf(stderr, "Usage: %s --file <string> --port <int> [--verbose]\n", argv0);
}

int main(int argc, char **argv) {
    struct options opts = {0};
    opts.verbose = false;

    static struct option long_options[] = {
        {"port",    required_argument, 0, 'p'},
        {"file",    required_argument, 0, 'f'},
        {"verbose", no_argument,       0, 'v'},
        {NULL,      0,                 0, 0}
    };

    int opt = 0;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "p:f:v", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                opts.port = atoi(optarg);
                break;
            case 'f':
                opts.file = optarg;
                break;
            case 'v':
                opts.verbose = true;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (opts.port == 0 || opts.file == NULL) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(opts.port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        close(sock);
        error_exit("Unable to set socket option");
    }

    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(sock);
        error_exit("Unable to bind socket");
    }

    if (listen(sock, 1) < 0) {
        close(sock);
        error_exit("Unable to listen");
    }

    printf("Speedtest server is listening on port %d\n", opts.port);
    struct sockaddr_in client_address;
    socklen_t addr_len = sizeof(client_address);
    int client = accept(sock, (struct sockaddr *)&client_address, &addr_len);
    if (client < 0) {
        close(sock);
        error_exit("Unable to accept client");
    }

    uint64_t T1, T2, T3, T4;
    if (recv(client, &T1, sizeof(T1), 0) <= 0) {
        close(client);
        close(sock);
        error_exit("Time sync: recv failed");
    }

    T2 = get_time_ns();
    if (send(client, &T2, sizeof(T2), 0) < 0) {
        close(client);
        close(sock);
        error_exit("Time sync: send failed");
    }
    if (recv(client, &T3, sizeof(T3), 0) <= 0) {
        close(client);
        close(sock);
        error_exit("Time recv: recv failed");
    }

    T4 = get_time_ns();
    if (send(client, &T4, sizeof(T4), 0) < 0) {
        close(client);
        close(sock);
        error_exit("Time send: recv failed");
    }

    FILE *log_file = fopen(opts.file, "w");
    if (!log_file) {
        close(client);
        close(sock);
        error_exit("Unable to open log file");
    }

    fprintf(log_file, "send_time,receive_time,sock_rtt,sock_cwnd,progress\n");

    if (setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &(int) {1}, sizeof(int)) < 0) {
        close(client);
        close(sock);
        error_exit("Unable to set socket option");
    }

    uint64_t results_capacity = RESULTS_INIT_SIZE;
    uint64_t results_index = 0;
    speedtest_packet_header_t *results = malloc(RESULTS_INIT_SIZE * sizeof(speedtest_packet_header_t) + PACKET_SIZE);
    if (results == NULL) {
        close(client);
        close(sock);
        error_exit("malloc failed");
        return 0;
    }

    const uint64_t t0 = get_time_ns();

    speedtest_packet_t *packet = (speedtest_packet_t *) results;

    while (recv(client, packet, sizeof(speedtest_packet_t), MSG_WAITALL) > 0) {
        packet->header.recv_time_ns = get_time_ns() - t0;
        results_index++;

        if (results_index == results_capacity) {
            results_capacity += RESULTS_INCREMENT;
            printf("Doing a realloc to %ld, this could be slow.\n", results_capacity);
            results = realloc(results, results_capacity * sizeof(speedtest_packet_header_t) + PACKET_SIZE);
            if (results == NULL) {
                close(client);
                close(sock);
                error_exit("realloc failed");
            }
        }

        packet = (speedtest_packet_t *) ((uint64_t)results + (uint64_t)(results_index * sizeof(speedtest_packet_header_t)));
    }

    close(client);
    close(sock);

    for (uint64_t i = 0; i < results_index; i++) {
        speedtest_packet_header_t *header = &results[i];
        fprintf(log_file, "%lu,%lu,%u,%u,%lu\n",
            header->send_time_ns,
            header->recv_time_ns,
            header->rtt,
            header->cwnd,
            header->acknowledged_bytes
        );
    }
    
    free(results);
    fclose(log_file);
}
