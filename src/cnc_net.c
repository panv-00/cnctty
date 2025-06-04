#include "cnc_net.h"

cnc_net *cn_init()
{
  cnc_net *n = malloc(sizeof(cnc_net));

  if (n == NULL)
  {
    return NULL;
  }

  n->connected = false;
  n->sockfd    = -1;
  n->ctx       = NULL;
  n->ssl       = NULL;

  n->app = NULL;

  // ignore SIGPIPE
  signal(SIGPIPE, SIG_IGN);

  return n;
}

bool cn_connect(cnc_net *n)
{
  if (n == NULL)
  {
    return false;
  }

  if (n->connected)
  {
    return true;
  }

  // unsigned long err = ERR_get_error();
  // fprintf(stderr, "SSL error: %s\n", ERR_error_string(err, NULL));

  // SSL_library_init();
  // SSL_load_error_strings();

  const SSL_METHOD *method = TLS_client_method();
  n->ctx                   = SSL_CTX_new(method);

  // n->ctx = SSL_CTX_new(SSLv23_client_method());

  if (n->ctx == NULL)
  {
    n->connected = false;

    return false;
  }

  n->sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (n->sockfd < 0)
  {
    n->connected = false;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  memset(&n->serv_addr, 0, sizeof(n->serv_addr));

  n->serv_addr.sin_family = AF_INET;
  n->serv_addr.sin_port   = htons(PORT);

  if (inet_pton(AF_INET, ADDRESS, &n->serv_addr.sin_addr) <= 0)
  {
    n->connected = false;

    close(n->sockfd);
    n->sockfd = -1;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  if (connect(n->sockfd, (struct sockaddr *)&n->serv_addr,
              sizeof(n->serv_addr)) < 0)
  {
    n->connected = false;

    close(n->sockfd);
    n->sockfd = -1;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  n->ssl = SSL_new(n->ctx);

  if (n->ssl == NULL)
  {
    n->connected = false;

    close(n->sockfd);
    n->sockfd = -1;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  if (SSL_set_fd(n->ssl, n->sockfd) != 1)
  {
    n->connected = false;

    close(n->sockfd);
    n->sockfd = -1;

    SSL_free(n->ssl);
    n->ssl = NULL;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  if (SSL_connect(n->ssl) != 1)
  {
    n->connected = false;

    close(n->sockfd);
    n->sockfd = -1;

    SSL_free(n->ssl);
    n->ssl = NULL;

    SSL_CTX_free(n->ctx);
    n->ctx = NULL;

    return false;
  }

  n->connected = true;

  return true;
}

int cn_receive(cnc_net *n)
{
  if (n == NULL ||             // n is null
      n->connected == false || // network is disconnected
      n->ctx == NULL ||        // ctx is null
      n->ssl == NULL ||        // ssl is null
      n->sockfd == -1)         // socket fd is not set
  {
    return -1;
  }

  char buffer[BUFSIZE];

  int bytes_received = SSL_read(n->ssl, buffer, sizeof(buffer));

  if (bytes_received <= 0)
  {
    cn_disconnect(n);

    return -1;
  }

  cm_add_buffer_to_messages(buffer, bytes_received, n->msg_buffer, n->username,
                            n->app);

  return bytes_received;
}

int cn_send(cnc_net *n, const char *data)
{
  if (n == NULL ||             // n is null
      data == NULL ||          // data pointer is null
      n->connected == false || // network is disconnected
      n->ctx == NULL ||        // ctx is null
      n->ssl == NULL ||        // ssl is null
      n->sockfd == -1)         // socket fd is not set
  {
    return -1;
  }

  int bytes_sent = SSL_write(n->ssl, data, strlen(data) + 1);

  if (bytes_sent <= 0)
  {
    cn_disconnect(n);

    return -1;
  }

  return bytes_sent;
}

void cn_disconnect(cnc_net *n)
{
  n->connected = false;

  if (n == NULL)
  {
    return;
  }

  // Shutdown SSL connection
  if (n->ssl)
  {
    SSL_write(n->ssl, ".q", 3);
    sleep(1);

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
}

void cn_destroy(cnc_net **n_ptr)
{
  if (n_ptr == NULL || *n_ptr == NULL)
  {
    return;
  }

  // make sure network is disconnected
  cn_disconnect(*n_ptr);

  free(*n_ptr);
  *n_ptr = NULL;
}
