#define RB_C

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "tcp.h"
#include "rb.h"

static const char *TAG = "Rb";

static const char *MESSAGE_NAMES[MAX_RB_MSG_TYPE] = {
    [RB_PULSE] = "pulse",
    [RB_BUTTON_UP] = "btUp",
    [RB_BUTTON_DOWN] = "btDown",
    [RB_BUTTON_UP_10] = "btUp10",
    [RB_BUTTON_DOWN_10] = "btDown10",
    [RB_LAP] = "lap",
    [RB_CKM] = "ckm",
};

/**
 * @brief Format a rb frame into a string buffer
 * @param frame The rb frame to format
 * @param buf The buffer to write the formatted string into
 * @param buflen The length of the buffer
 * @return The length of the formatted string
 */
static size_t format_frame(const rb_frame_t *frame, char *buf, size_t buflen)
{
    if (frame->type >= MAX_RB_MSG_TYPE)
        return 0;

    if (frame->type == RB_PULSE)
        snprintf(buf, buflen, "%s=%d;%d;%u;00|\n", MESSAGE_NAMES[frame->type], frame->s1, frame->s2, frame->timestamp);
    else
        snprintf(buf, buflen, "%s=%d;%d|\n", MESSAGE_NAMES[frame->type], frame->s1, frame->s2);

    return strlen(buf);
}

/**
 * @brief Initialize the Rb application and start the TCP server
 */
void rb_start(void)
{
    tcp_server_init();
}

/**
 * @brief Stop the Rb application and deinitialize the TCP server
 */
void rb_stop(void)
{
    tcp_server_deinit();
}

/**
 * @brief Create a pulse frame with the given parameters
 * @param s1 The first sensor value
 * @param s2 The second sensor value
 * @param timestamp The timestamp of the pulse
 * @return The created rb frame
 */
rb_frame_t create_pulse_frame(int s1, int s2, unsigned int timestamp)
{
    rb_frame_t frame;
    frame.type = RB_PULSE;
    frame.s1 = s1;
    frame.s2 = s2;
    frame.timestamp = timestamp;
    /* The Flags param can be added if the purpose is known in the future */

    return frame;
}

/**
 * @brief Create a button frame with the given parameters
 * @param type The type of button event (up/down)
 * @param s1 The first sensor value
 * @param s2 The second sensor value
 * @return The created rb frame
 */
rb_frame_t create_button_frame(rb_msg_type_t type, int s1, int s2)
{
    rb_frame_t frame = { 0 };
    frame.type = type;
    frame.s1 = s1;
    frame.s2 = s2;
    return frame;
}

/**
 * @brief Send a rb frame to all connected TCP clients
 * @param frame The rb frame to send
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t rb_send_frame(const rb_frame_t *frame)
{
    char buf[64];
    size_t len = format_frame(frame, buf, sizeof(buf));

    if (len > 0) {
        tcp_server_send(buf, len);
        ESP_LOGD(TAG, "Sent frame: %s", buf);
        return ESP_OK;
    }
    else {
        ESP_LOGE(TAG, "Failed to format frame");
        return ESP_ERR_INVALID_ARG;
    }
}
