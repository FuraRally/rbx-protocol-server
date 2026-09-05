#define TCP_C

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "tcp.h"

#define MAX_TCP_CONNECTIONS CONFIG_RBX_SERVER_MAX_CONNECTIONS
#define PORT                CONFIG_RBX_SERVER_PORT
#define KEEPALIVE_IDLE      60
#define KEEPALIVE_INTERVAL  10
#define KEEPALIVE_COUNT     5

#define LISTENER_TASK_STACK_SIZE 1024 * 4
#define CLIENT_TASK_STACK_SIZE   1024 * 4

static const char *TAG = "tcp_server";

static int listen_sock = -1;
/* Array to hold client sockets */
static int client_socks[MAX_TCP_CONNECTIONS] = { 0 };

/**
 * @brief Remove client socket from array
 * @param sock The socket to remove
 */
static void tcp_remove_client_sock(int sock)
{
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i) {
        if (client_socks[i] == sock) {
            client_socks[i] = -1;
            break;
        }
    }
}

/**
 * @brief Add client socket to array
 * @param sock The socket to add
 * @return 0 on success, -1 if no space available
 */
static int tcp_add_client_sock(int sock)
{
    /* Check if socket is already in array */
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i) {
        if (client_socks[i] == sock) {
            return 0;
        }
    }

    /* Find an empty slot for the new socket */
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i) {
        if (client_socks[i] == -1) {
            client_socks[i] = sock;
            return 0;
        }
    }

    return -1;
}

/**
 * @brief Check if socket is valid
 * @param sock The socket to check
 * @return 1 if valid, 0 if invalid
 */
static int is_sock_valid(int sock)
{
    return sock >= 0;
}

/**
 * @brief Close client socket and remove from array
 * @param sock The socket to close
 */
static int tcp_close_client_sock(int sock)
{
    if (!is_sock_valid(sock))
        return -1;

    shutdown(sock, 0);
    close(sock);
    tcp_remove_client_sock(sock);
    return 0;
}

/**
 * @brief Task to handle communication with a single client
 * @param pvParameters The client socket passed as parameter
 */
static void tcp_client_task(void *pvParameters)
{
    int sock = (int)pvParameters;
    char rx_buffer[128];

    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
            break;
        }
        else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed by client");
            break;
        }
        else {
            /* Currently we don't do anything with received data, but we could log it or handle commands here */
            rx_buffer[len] = 0;
            ESP_LOGD(TAG, "Received %d bytes: %s", len, rx_buffer);
        }
    }

    /* If loop exits, connection is closed so we clean up the socket and delete the task */
    tcp_close_client_sock(sock);
    vTaskDelete(NULL);
}

/**
 * @brief Create a task to handle a new client connection
 * @param sock The client socket
 * @return 0 on success, -1 on failure
 */
static int tcp_create_client_task(int sock)
{
    if (tcp_add_client_sock(sock) < 0) {
        ESP_LOGW(TAG, "Max connections reached, closing new client");
        return -1;
    }

    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;

    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(keepAlive));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));

    xTaskCreate(tcp_client_task, "tcp_client", CLIENT_TASK_STACK_SIZE, (void *)sock, 5, NULL);

    return 0;
}

/**
 * @brief Initialize the TCP server task that listens for incoming connections
 */
static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];

    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
    };

    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGD(TAG, "Socket created");

    if (bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGD(TAG, "Socket bound, port %d", PORT);

    if (listen(listen_sock, MAX_TCP_CONNECTIONS) != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    ESP_LOGI(TAG, "Socket listening");
    while (1) {
        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        /* Create a task to handle this client connection */
        if (tcp_create_client_task(sock) < 0) {
            ESP_LOGE(TAG, "Unable to create client task for new connection");
            shutdown(sock, 0);
            close(sock);
            continue;
        }

        if (source_addr.ss_family == PF_INET)
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);

        ESP_LOGD(TAG, "Socket accepted ip address: %s", addr_str);
    }

CLEAN_UP:
    close(listen_sock);
    listen_sock = -1;
    vTaskDelete(NULL);
}

/**
 * @brief Initialize the TCP server task that listens for incoming connections
 */
void tcp_server_init(void)
{
    /* Initialize client sockets as free slots */
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i)
        client_socks[i] = -1;

    xTaskCreate(tcp_server_task, "tcp_server", LISTENER_TASK_STACK_SIZE, NULL, 5, NULL);
}

/**
 * @brief Deinitialize the TCP server and close all client connections
 */
void tcp_server_deinit(void)
{
    /* All tasks will be deleted automatically when their sockets are closed */

    /* Close all client sockets */
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i)
        tcp_close_client_sock(client_socks[i]);

    /* Close the listening socket */
    if (is_sock_valid(listen_sock)) {
        close(listen_sock);
        listen_sock = -1;
    }
}

/**
 * @brief Send data to all connected TCP clients
 * @param buf The buffer containing the data to send
 * @param len The length of the data to send
 */
void tcp_server_send(const char *buf, size_t len)
{
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i) {
        if (!is_sock_valid(client_socks[i]))
            continue;

        int to_write = len;
        int offset = 0;

        while (to_write > 0) {
            int written = send(client_socks[i], buf + offset, to_write, 0);
            if (written < 0) {
                ESP_LOGE(TAG, "Send error to client %d: errno %d", client_socks[i], errno);
                tcp_remove_client_sock(client_socks[i]);
                break;
            }
            to_write -= written;
            offset += written;
        }
    }
}

/**
 * @brief Get the number of currently connected TCP clients
 * @return The number of connected clients
 */
int tcp_server_get_client_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_TCP_CONNECTIONS; ++i) {
        if (is_sock_valid(client_socks[i]))
            count++;
    }
    return count;
}
