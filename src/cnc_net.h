#ifndef CNC_NET_H
#define CNC_NET_H

#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "cnc_library/src/lib/cnc_library.h"
#include "cnc_message.h"

// we will use cn as prefix for cnc_net

// uncomment below if run in development mode
// #define DEVEL_MODE 1

#ifdef DEVEL_MODE
#define ADDRESS "127.0.0.1"
#else
#define ADDRESS "76.235.94.33"
#endif

#define PORT    6667
#define BUFSIZE 2048

typedef struct
{
  bool               connected;
  int                sockfd;
  struct sockaddr_in serv_addr;
  SSL_CTX           *ctx;
  SSL               *ssl;

  cnc_buffer *username;
  cnc_buffer *msg_buffer;
  cnc_app    *app;

} cnc_net;

cnc_net *cn_init();
bool     cn_connect(cnc_net *n);
int      cn_receive(cnc_net *n);
int      cn_send(cnc_net *n, const char *data);
void     cn_disconnect(cnc_net *n);
void     cn_destroy(cnc_net **n_ptr);

#endif
