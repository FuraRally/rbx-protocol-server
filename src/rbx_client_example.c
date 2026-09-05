#include "rbx_client.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    int sockfd = rbx_client_connect(host, port);
    if (sockfd < 0) {
        printf("Failed to connect to %s:%d\n", host, port);
        return 1;
    }
    printf("Connected to %s:%d\n", host, port);
    rbx_client_frame_t frame;
    while (rbx_client_recv_frame(sockfd, &frame) > 0) {
        printf("Received: type=%d, s1=%d, s2=%d, timestamp=%u\n", frame.type, frame.s1, frame.s2, frame.timestamp);
    }
    printf("Disconnected.\n");
    rbx_client_disconnect(sockfd);
    return 0;
}
