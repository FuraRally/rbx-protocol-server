#ifndef RB_H
#define RB_H

#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Macros to get the frame with some data */
#define RB_FRAME_PULSE(sensor1, sensor2, time)                                                                     \
    {                                                                                                                  \
        .type = RB_PULSE, .s1 = sensor1, .s2 = sensor2, .timestamp = time,                                         \
    }

#define RB_FRAME_BUTTON(buttonType, sensor1, sensor2)                                                              \
    {                                                                                                                  \
        .type = buttonType, .s1 = sensor1, .s2 = sensor2,                                                              \
        .timestamp = 0, /* Timestamp is not used for button frames */                                                  \
    }

/* Maximum number of frames the queue can hold and the maximum batch size for rb_send_multiple_frames */
#define RB_QUEUE_MAX_FRAMES  CONFIG_RBX_SERVER_QUEUE_SIZE        

/* Interval in milliseconds at which the frame queue is flushed and broadcast to all clients */
#define RB_QUEUE_FLUSH_INTERVAL_MS  CONFIG_RBX_SERVER_QUEUE_FLUSH_INTERVAL

/* Maximum size in bytes of a single formatted message (including the trailing '|') */
#define RB_MAX_FRAME_MSG_SIZE       64

/* Message types that the rb app can handle */
typedef enum
{
    RB_PULSE,
    RB_BUTTON_UP,
    RB_BUTTON_DOWN,
    RB_BUTTON_UP_10,
    RB_BUTTON_DOWN_10,
    RB_LAP,
    RB_CKM,
    MAX_RB_MSG_TYPE,
} rb_msg_type_t;

typedef struct
{
    rb_msg_type_t type;
    int s1;
    int s2;
    unsigned int timestamp; /* Only for pulse */
} rb_frame_t;

/* Functions */
void rb_start(void);
void rb_stop(void);

rb_frame_t create_pulse_frame(int s1, int s2, unsigned int timestamp);
rb_frame_t create_button_frame(rb_msg_type_t type, int s1, int s2);

esp_err_t rb_send_frame(const rb_frame_t *frame);
esp_err_t rb_send_multiple_frames(const rb_frame_t *frames, size_t length);
esp_err_t rb_queue_frame(const rb_frame_t *frame);

#endif /* RB_H */
