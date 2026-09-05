#define RB_C

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

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

static QueueHandle_t rb_frame_queue = NULL;
static TimerHandle_t rb_flush_timer = NULL;

/**
 * @brief Format a single message into buf without a trailing newline (ends with '|')
 * @param frame The rb frame to format
 * @param buf The buffer to write the formatted string into
 * @param buflen The length of the buffer
 * @return The length of the formatted string, or 0 on error
 */
static size_t format_message(const rb_frame_t *frame, char *buf, size_t buflen)
{
    if (frame->type >= MAX_RB_MSG_TYPE)
        return 0;

    if (frame->type == RB_PULSE)
        snprintf(buf, buflen, "%s=%d;%d;%u;00|", MESSAGE_NAMES[frame->type], frame->s1, frame->s2, frame->timestamp);
    else
        snprintf(buf, buflen, "%s=%d;%d|", MESSAGE_NAMES[frame->type], frame->s1, frame->s2);

    return strlen(buf);
}

/**
 * @brief Format a rb frame into a complete protocol frame string (ends with '|\n')
 * @param frame The rb frame to format
 * @param buf The buffer to write the formatted string into
 * @param buflen The length of the buffer
 * @return The length of the formatted string, or 0 on error
 */
static size_t format_frame(const rb_frame_t *frame, char *buf, size_t buflen)
{
    size_t len = format_message(frame, buf, buflen);

    if (len > 0 && len < buflen - 1) {
        buf[len] = '\n';
        buf[len + 1] = '\0';
        len++;
    }

    return len;
}

/**
 * @brief Timer callback that flushes all queued frames as a single concatenated frame
 * @param xTimer The timer handle (unused)
 */
static void rb_flush_queue_cb(TimerHandle_t xTimer)
{
    if (uxQueueMessagesWaiting(rb_frame_queue) == 0)
        return;

    char buf[RB_QUEUE_MAX_FRAMES * RB_MAX_FRAME_MSG_SIZE + 2];
    char msg[RB_MAX_FRAME_MSG_SIZE];
    size_t total = 0;
    rb_frame_t frame;

    while (xQueueReceive(rb_frame_queue, &frame, 0) == pdTRUE) {
        size_t len = format_message(&frame, msg, sizeof(msg));
        if (len == 0)
            continue;
        if (total + len >= sizeof(buf) - 1)
            break;
        memcpy(buf + total, msg, len);
        total += len;
    }

    if (total > 0) {
        buf[total] = '\n';
        total++;
        buf[total] = '\0';
        tcp_server_send(buf, total);
        ESP_LOGD(TAG, "Flushed queue: %s", buf);
    }
}

/**
 * @brief Initialize the Rb application, start the TCP server and the frame queue with its flush timer
 */
void rb_start(void)
{
    rb_frame_queue = xQueueCreate(RB_QUEUE_MAX_FRAMES, sizeof(rb_frame_t));
    if (rb_frame_queue == NULL)
        ESP_LOGE(TAG, "Failed to create frame queue");

    rb_flush_timer = xTimerCreate("rb_flush", pdMS_TO_TICKS(RB_QUEUE_FLUSH_INTERVAL_MS), pdTRUE, NULL, rb_flush_queue_cb);
    if (rb_flush_timer != NULL)
        xTimerStart(rb_flush_timer, 0);
    else
        ESP_LOGE(TAG, "Failed to create flush timer");

    tcp_server_init();
}

/**
 * @brief Stop the Rb application, deinitialize the TCP server and clean up the frame queue
 */
void rb_stop(void)
{
    if (rb_flush_timer != NULL) {
        xTimerStop(rb_flush_timer, portMAX_DELAY);
        xTimerDelete(rb_flush_timer, portMAX_DELAY);
        rb_flush_timer = NULL;
    }

    if (rb_frame_queue != NULL) {
        vQueueDelete(rb_frame_queue);
        rb_frame_queue = NULL;
    }

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
    char buf[RB_MAX_FRAME_MSG_SIZE + 1];
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

/**
 * @brief Send multiple rb frames as a single concatenated protocol frame to all connected TCP clients
 * @param frames Array of rb frames to send
 * @param length Number of frames in the array
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if any frame is invalid, ESP_ERR_NO_MEM if buffer overflows
 */
esp_err_t rb_send_multiple_frames(const rb_frame_t *frames, size_t length)
{
    char buf[RB_QUEUE_MAX_FRAMES * RB_MAX_FRAME_MSG_SIZE + 2];
    char msg[RB_MAX_FRAME_MSG_SIZE];
    size_t total = 0;

    for (size_t i = 0; i < length; i++) {
        size_t len = format_message(&frames[i], msg, sizeof(msg));
        if (len == 0) {
            ESP_LOGE(TAG, "Failed to format frame %zu", i);
            return ESP_ERR_INVALID_ARG;
        }
        if (total + len >= sizeof(buf) - 1) {
            ESP_LOGE(TAG, "Buffer overflow in rb_send_multiple_frames");
            return ESP_ERR_NO_MEM;
        }
        memcpy(buf + total, msg, len);
        total += len;
    }

    if (total == 0)
        return ESP_ERR_INVALID_ARG;

    buf[total] = '\n';
    total++;
    buf[total] = '\0';

    tcp_server_send(buf, total);
    ESP_LOGD(TAG, "Sent %zu frames: %s", length, buf);
    return ESP_OK;
}

/**
 * @brief Add a rb frame to the queue for periodic broadcasting
 * @param frame The rb frame to enqueue
 * @return ESP_OK on success, ESP_ERR_INVALID_STATE if queue is not initialized, ESP_ERR_NO_MEM if queue is full
 */
esp_err_t rb_queue_frame(const rb_frame_t *frame)
{
    if (rb_frame_queue == NULL) {
        ESP_LOGE(TAG, "Frame queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(rb_frame_queue, frame, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Frame queue full");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
