#ifndef RBX_CLIENT_H
#define RBX_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Protocol message types
typedef enum {
    RBX_CLIENT_PULSE,
    RBX_CLIENT_BUTTON_UP,
    RBX_CLIENT_BUTTON_DOWN,
    RBX_CLIENT_BUTTON_UP_10,
    RBX_CLIENT_BUTTON_DOWN_10,
    RBX_CLIENT_LAP,
    RBX_CLIENT_CKM,
    RBX_CLIENT_UNKNOWN
} rbx_client_msg_type_t;

// Frame structure
typedef struct {
    rbx_client_msg_type_t type;
    int s1;
    int s2;
    unsigned int timestamp; // Only for pulse
} rbx_client_frame_t;

// Connect to server (returns socket fd or -1)
int rbx_client_connect(const char *host, uint16_t port);
// Disconnect from server
void rbx_client_disconnect(int sockfd);
// Receive and parse next frame (blocking)
// Returns 1 on success, 0 on disconnect, -1 on error
int rbx_client_recv_frame(int sockfd, rbx_client_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif // RBX_CLIENT_H
