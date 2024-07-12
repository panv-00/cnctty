#ifndef CNC_NET_H
#define CNC_NET_H

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "cnc_message.h"

// uncomment below if run in development mode
// #define DEVEL_MODE 1

// network errors
#define NO_NET_ERROR 0
#define SOCKET_ERROR 100
#define SERVER_ERROR 200
#define SSL_ERROR 300
#define ERROR_RECEIVING_DATA 400
#define CONNECTION_CLOSED 500

#ifdef DEVEL_MODE
#define ADDRESS "127.0.0.1"
#else
#define ADDRESS "76.235.94.33"
#endif

#define PORT 6667
#define BUFSIZE 2048

typedef struct
{
  bool connected;
  int sockfd;
  struct sockaddr_in serv_addr;
  SSL_CTX *ctx;
  SSL *ssl;

  bool stop_receiving;

  cnc_buffer *databuffer;

  // pointers to variables in other modules
  cnc_buffer *username;
  cnc_buffer *message_buffer;
  cnc_terminal *terminal;
  cnc_widget *infobar;

} cnc_net;

cnc_net *cnc_net_init();
int cnc_net_connect(cnc_net *n);
int cnc_net_receive(cnc_net *n);
void cnc_net_disconnect(cnc_net *n);
void cnc_net_destroy(cnc_net *n);

#endif
