#include "rbx_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RBX_CLIENT_BUFSIZE 128

static rbx_client_msg_type_t parse_type(const char *name) {
    if (strcmp(name, "pulse") == 0) return RBX_CLIENT_PULSE;
    if (strcmp(name, "btUp") == 0) return RBX_CLIENT_BUTTON_UP;
    if (strcmp(name, "btDown") == 0) return RBX_CLIENT_BUTTON_DOWN;
    if (strcmp(name, "btUp10") == 0) return RBX_CLIENT_BUTTON_UP_10;
    if (strcmp(name, "btDown10") == 0) return RBX_CLIENT_BUTTON_DOWN_10;
    if (strcmp(name, "lap") == 0) return RBX_CLIENT_LAP;
    if (strcmp(name, "ckm") == 0) return RBX_CLIENT_CKM;
    return RBX_CLIENT_UNKNOWN;
}

int rbx_client_connect(const char *host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void rbx_client_disconnect(int sockfd) {
    if (sockfd >= 0) close(sockfd);
}

static int parse_frame(const char *msg, rbx_client_frame_t *frame) {
    char type[16];
    int n;
    if (sscanf(msg, "%15[^=]=%n", type, &n) != 1) return -1;
    msg += n;
    if (strcmp(type, "pulse") == 0) {
        int s1, s2;
        unsigned int ts;
        if (sscanf(msg, "%d;%d;%u;00|", &s1, &s2, &ts) == 3) {
            frame->type = RBX_CLIENT_PULSE;
            frame->s1 = s1;
            frame->s2 = s2;
            frame->timestamp = ts;
            return 1;
        }
    } else {
        int s1, s2;
        if (sscanf(msg, "%d;%d|", &s1, &s2) == 2) {
            frame->type = parse_type(type);
            frame->s1 = s1;
            frame->s2 = s2;
            frame->timestamp = 0;
            return 1;
        }
    }
    return -1;
}

int rbx_client_recv_frame(int sockfd, rbx_client_frame_t *frame) {
    char buf[RBX_CLIENT_BUFSIZE];
    int pos = 0;
    while (1) {
        int n = read(sockfd, buf + pos, 1);
        if (n <= 0) return n;
        if (buf[pos] == '\n') {
            buf[pos + 1] = 0;
            break;
        }
        if (++pos >= RBX_CLIENT_BUFSIZE - 1) return -1;
    }
    // Parse each message separated by '|'
    char *saveptr = NULL;
    char *msg = strtok_r(buf, "|", &saveptr);
    int found = 0;
    while (msg) {
        if (parse_frame(msg, frame) == 1) {
            found = 1;
            break; // Only return the first valid frame
        }
        msg = strtok_r(NULL, "|", &saveptr);
    }
    return found ? 1 : -1;
}
