#include "cnc_net.h"

cnc_net *cnc_net_init()
{
  cnc_net *n = (cnc_net *)malloc(sizeof(cnc_net));

  if (!n)
  {
    return NULL;
  }

  n->connected = false;
  n->sockfd = -1;
  n->ctx = NULL;
  n->ssl = NULL;

  n->databuffer = NULL;

  n->username = NULL;
  n->message_buffer = NULL;
  n->terminal = NULL;
  n->infobar = NULL;

  // ignore SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  return n;
}

int cnc_net_connect(cnc_net *n)
{
  SSL_library_init();
  SSL_load_error_strings();

  n->ctx = SSL_CTX_new(SSLv23_client_method());

  if (n->ctx == NULL)
  {
    return -1;
  }

  n->sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (n->sockfd < 0)
  {
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return -1;
  }

  memset(&n->serv_addr, 0, sizeof(n->serv_addr));
  n->serv_addr.sin_family = AF_INET;
  n->serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, ADDRESS, &n->serv_addr.sin_addr) <= 0)
  {
    close(n->sockfd);
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return -1;
  }

  if (connect(n->sockfd, (struct sockaddr *)&n->serv_addr,
              sizeof(n->serv_addr)) < 0)
  {
    close(n->sockfd);
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return -1;
  }

  n->ssl = SSL_new(n->ctx);

  if (n->ssl == NULL)
  {
    close(n->sockfd);
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return -1;
  }

  SSL_set_fd(n->ssl, n->sockfd);

  if (SSL_connect(n->ssl) != 1)
  {
    SSL_free(n->ssl);
    close(n->sockfd);
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return -1;
  }

  n->connected = true;

  return 0;
}

int cnc_net_receive(cnc_net *n)
{
  char buffer[BUFSIZE];

  int bytes_received = SSL_read(n->ssl, buffer, sizeof(buffer));

  if (bytes_received <= 0)
  {
    n->connected = false;
    cnc_net_disconnect(n);

    return -1;
  }

  add_buffer_to_messages(buffer, bytes_received, n->message_buffer,
                         n->username->contents, n->databuffer, n->terminal,
                         n->infobar);

  return bytes_received;
}

int cnc_net_send(cnc_net *n, const char *data)
{
  int bytes_sent = SSL_write(n->ssl, data, calen(data) + 1);

  if (bytes_sent <= 0)
  {
    n->connected = false;
    cnc_net_disconnect(n);

    return -1;
  }

  return bytes_sent;
}

void cnc_net_disconnect(cnc_net *n)
{
  if (n == NULL)
  {
    return;
  }

  SSL_write(n->ssl, ".q", 3);
  sleep(1);

  // Shutdown SSL connection
  if (n->ssl != NULL)
  {
    SSL_shutdown(n->ssl);
    SSL_free(n->ssl);
    n->ssl = NULL;
  }

  // Close socket
  if (n->sockfd >= 0)
  {
    close(n->sockfd);
    n->sockfd = -1;
  }

  // Free SSL
  if (n->ctx != NULL)
  {
    SSL_CTX_free(n->ctx);
    n->ctx = NULL;
  }

  // Update connection status
  n->connected = false;
}

void cnc_net_destroy(cnc_net *n)
{
  while (n->connected)
  {
    cnc_net_disconnect(n);
    sleep(1);
  }

  free(n);
  n = NULL;
}
