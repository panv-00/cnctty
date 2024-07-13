#include "cnctty.h"

void set_info(cnc_widget *i, const char *text, char *bg, cnc_widget *p,
              cnc_widget *d)
{
  char buf[14];

  sprintf(buf, "[%5zu/%5zu]", d->data->length, d->data->size);

  cnc_buffer_set_text(i->data, buf);

  if (p->has_focus)
  {
    cnc_buffer_append(i->data, " PROMPT : ");
  }

  else if (d->has_focus)
  {
    cnc_buffer_append(i->data, " DISPLAY: ");
  }

  else
  {
    cnc_buffer_append(i->data, "          ");
  }

  cnc_buffer_append(i->data, text);
  i->background = bg;
}

int connect_to_server(cnc_net *n)
{
  int retry_connect = 0;
  int max_retry_connect = 3;
  int net_connect_result = 0;

  while (!n->connected && retry_connect < max_retry_connect)
  {
    retry_connect++;
    net_connect_result = cnc_net_connect(n);

    if (net_connect_result != NO_NET_ERROR)
    {
      sleep(1);
    }
  }

  return net_connect_result;
}

void *receive_net_data(void *arg)
{
  ThreadData *data = (ThreadData *)arg;
  cnc_net *net = data->net;
  bool *thread_completed = data->thread_completed;

  cnc_net_receive(net);

  *thread_completed = true;
  pthread_exit(NULL);
}

void disconnect_from_server(cnc_net *net, cnc_widget *infobar,
                            cnc_terminal *term, cnc_widget *display,
                            cnc_widget *prompt)
{
  if (net->connected)
  {
    set_info(infobar, "disconnecting...", COLOR_WHITE_BG, prompt, display);
    cnc_terminal_update_and_redraw(term);

    cnc_net_disconnect(net);
    set_info(infobar, "offline", COLOR_RED_BG, prompt, display);
  }
}

void free_all_allocations(cnc_terminal *term, cnc_net *net,
                          cnc_buffer *username, cnc_buffer *password,
                          cnc_buffer *message_buffer)
{
  if (username)
  {
    cnc_buffer_destroy(username);
  }

  if (password)
  {
    cnc_buffer_destroy(password);
  }

  if (message_buffer)
  {
    cnc_buffer_destroy(message_buffer);
  }

  if (net)
  {
    cnc_net_destroy(net);
  }

  if (term)
  {
    cnc_terminal_destroy(term);
  }
}

int main(void)
{
  int user_input = 0;
  bool end_app = false;
  bool display_disconnected_message = true;
  int net_connect_result = 0;

  cnc_terminal *term = NULL;
  cnc_net *net = NULL;
  cnc_buffer *message_buffer = NULL;

  bool thread_completed = true;
  ThreadData data;
  pthread_t receiving_thread;

  cnc_buffer *username = cnc_buffer_init(MAX_NAME);
  cnc_buffer *password = cnc_buffer_init(MAX_NAME);

  term = cnc_terminal_init(TERM_MIN_WIDTH, TERM_MIN_HEIGHT);

  if (!term)
  {
    free_all_allocations(term, net, username, password, message_buffer);
    printf("could not initialize terminal!\n");

    exit(1);
  }

  cnc_widget *display = cnc_terminal_add_widget(term, WIDGET_DISPLAY);
  cnc_widget *infobar = cnc_terminal_add_widget(term, WIDGET_INFO);
  cnc_widget *prompt = cnc_terminal_add_widget(term, WIDGET_PROMPT);

  if (!display || !infobar || !prompt)
  {
    free_all_allocations(term, net, username, password, message_buffer);
    printf("could not add widgets!\n");

    exit(1);
  }

  if (!cnc_terminal_setup_widgets(term))
  {
    free_all_allocations(term, net, username, password, message_buffer);
    printf("could not setup widgets!\n");

    exit(1);
  }

  // initial screen setup
  cnc_buffer_set_text(display->data, "Welcome to cnctty v 1.01");
#ifdef DEVEL_MODE
  cnc_buffer_append(display->data, " (development mode)");
#endif
  set_info(infobar, "offline", COLOR_RED_BG, prompt, display);
  cnc_terminal_set_mode(term, MODE_INS);

  // initialize message buffer
  message_buffer = cnc_buffer_init(MAX_MESSAGE_BODY);

  if (!message_buffer)
  {
    free_all_allocations(term, net, username, password, message_buffer);
    printf("could not initialize message buffer!\n");

    exit(1);
  }

  // initialize the network
  net = cnc_net_init();

  if (!net)
  {
    free_all_allocations(term, net, username, password, message_buffer);
    printf("could not initialize network!\n");

    exit(1);
  }

  net->databuffer = display->data;
  net->message_buffer = message_buffer;
  net->terminal = term;
  net->infobar = infobar;
  net->username = username;

  // user interface begins here
  // initially get the username
  cnc_buffer_append(display->data, "\n    Pick a username: _\n");
  cnc_terminal_update_and_redraw(term);

  while (true)
  {
    user_input = cnc_terminal_get_user_input(term);

    if (user_input == ENTER_KEY && prompt->data->length > 0)
    {
      cnc_buffer_set_text(username, prompt->data->contents);
      cnc_widget_reset(prompt);
      break;
    }
  }

  // now get the password
  cnc_buffer_delete_char(display->data, display->data->length - 1);
  cnc_buffer_delete_char(display->data, display->data->length - 1);
  cnc_buffer_append(display->data, username->contents);
  cnc_buffer_append(display->data, "\n    Password ? _\n");
  cnc_terminal_update_and_redraw(term);

  while (true)
  {
    user_input = cnc_terminal_get_user_input(term);

    if (user_input == ENTER_KEY)
    {
      if (prompt->data->length > 0)
      {
        cnc_buffer_set_text(password, prompt->data->contents);
      }

      cnc_widget_reset(prompt);
      break;
    }
  }

  cnc_buffer_delete_char(display->data, display->data->length - 1);
  cnc_buffer_delete_char(display->data, display->data->length - 1);
  cnc_buffer_append(display->data,
                    password->length > 0 ? "yes..\n" : "no...\n");

  // loop while app has not ended
  while (!end_app)
  {
    // focus changed: refresh the infobar
    if (user_input == TAB_KEY)
    {
      cnc_buffer_replace_text(infobar->data, 14, 10,
                              prompt->has_focus    ? "PROMPT :"
                              : display->has_focus ? "DISPLAY:"
                                                   : "        ",
                              0);
    }

    // user wants to quit
    else if ((term->mode == MODE_INS && user_input == ENTER_KEY &&
              cnc_buffer_equal_string(prompt->data, ":q")) ||
             (term->mode == MODE_CMD && user_input == 'q'))
    {
      disconnect_from_server(net, infobar, term, display, prompt);
      end_app = true;
      cnc_widget_reset(prompt);
    }

    // user wants to connect
    else if ((term->mode == MODE_INS && user_input == ENTER_KEY &&
              cnc_buffer_equal_string(prompt->data, ":c")) ||
             (term->mode == MODE_CMD && user_input == 'c'))
    {
      if (!net->connected)
      {
        set_info(infobar, "connecting...", COLOR_WHITE_BG, prompt, display);
        cnc_terminal_update_and_redraw(term);
        net_connect_result = connect_to_server(net);

        if (net_connect_result == NO_NET_ERROR)
        {
          display_disconnected_message = true;
          set_info(infobar, "online", COLOR_GREEN_BG, prompt, display);

          // send username/password information to the server
          // this will be done if autoconnecting on app startup only.
          // it won't be necessary in later connects, maybe after disconnection

          cnc_buffer *connect_string = cnc_buffer_init(2 * MAX_NAME + 1);
          cnc_buffer_set_text(connect_string, ".n ");
          cnc_buffer_append(connect_string, username->contents);

          if (password->length > 0)
          {
            cnc_buffer_append(connect_string, "=");
            cnc_buffer_append(connect_string, password->contents);
          }

          SSL_write(net->ssl, connect_string->contents,
                    connect_string->length + 1);

          cnc_buffer_destroy(connect_string);

          // start receiving thread
          thread_completed = false;
          data.net = net;
          data.thread_completed = &thread_completed;

          pthread_create(&receiving_thread, NULL, receive_net_data,
                         (void *)&data);
          pthread_detach(receiving_thread);
        }
      }

      cnc_widget_reset(prompt);
    }

    // user wants to disconnect from server
    else if ((term->mode == MODE_INS && user_input == ENTER_KEY &&
              cnc_buffer_equal_string(prompt->data, ":d")) ||
             (term->mode == MODE_CMD && user_input == 'd'))
    {
      disconnect_from_server(net, infobar, term, display, prompt);
      cnc_widget_reset(prompt);
    }

    // user wants to send message to the server
    else if (term->mode == MODE_INS && user_input == ENTER_KEY)
    {
      if (net->connected)
      {
        // prompt->foreground = COLOR_WHITE_FG;
        // cnc_terminal_focus_widget(term, display);
        // cnc_terminal_update_and_redraw(term);
        // sleep(1);

        // changing username??
        if (prompt->data->contents[0] == '.' &&
            prompt->data->contents[1] == 'n')
        {
          cnc_buffer_clear(username);
          cnc_buffer_replace_text(username, 0, prompt->data->length - 2,
                                  prompt->data->contents, 2);
          cnc_buffer_trim(username);
        }

        SSL_write(net->ssl, prompt->data->contents, prompt->data->length + 1);
        // prompt->foreground = COLOR_CYAN_FG;
        // cnc_terminal_focus_widget(term, prompt);
        cnc_terminal_update_and_redraw(term);
      }

      cnc_widget_reset(prompt);
    }

    // handle disconnect status
    if (!net->connected)
    {
      if (display_disconnected_message)
      {
        cnc_buffer_append(display->data, "\n");
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_INFO_BYTE);
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_CODE_BLACK_BG);
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_INFO_BYTE);
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_CODE_YELLOW_FG);
        cnc_buffer_append(
            display->data,
            " You are disconnected. Input :c to connect or :q to quit\n");
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_INFO_BYTE);
        cnc_buffer_insert_char(display->data, display->data->length, 1,
                               COLOR_CODE_DEFAULT);
        cnc_buffer_append(display->data, "\n");

        display_disconnected_message = false;
      }

      set_info(infobar, "offline", COLOR_RED_BG, prompt, display);
    }

    cnc_terminal_update_and_redraw(term);

    if (!end_app)
    {
      user_input = cnc_terminal_get_user_input(term);
    }
  }

  // eventually, ensure disconnection from server before exit
  disconnect_from_server(net, infobar, term, display, prompt);

  // Wait for receiving thread to finish before exiting
  while (!thread_completed)
  {
    sleep(1);
  }

  free_all_allocations(term, net, username, password, message_buffer);

  printf("No errors reported. See you next time!\n");

  return 0;
}