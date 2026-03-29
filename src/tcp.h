#ifndef TCP_H
#define TCP_H

void tcp_server_init(void);
void tcp_server_deinit(void);
void tcp_server_send(const char *buf, size_t len);
int tcp_server_get_client_count(void);

#endif /* TCP_H */
