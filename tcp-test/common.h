#pragma once

#include <unistd.h>
#include <stdint.h>

#define PACKET_SIZE 1024

typedef struct speedtest_packet_header {
    uint64_t send_time_ns;
    uint64_t recv_time_ns;
    uint32_t rtt;
    uint32_t cwnd;
    uint64_t acknowledged_bytes;
} speedtest_packet_header_t;

typedef struct speedtest_packet {
    speedtest_packet_header_t header;
    char payload[PACKET_SIZE - sizeof(speedtest_packet_header_t)];
} speedtest_packet_t;

uint64_t get_time_ns();
uint64_t get_time_s();
void error_exit(const char *msg);
