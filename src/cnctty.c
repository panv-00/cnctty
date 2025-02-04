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

void usage(void)
{
  printf("Usage:\n");
  printf("$ cnctty --version: display version and exit.\n");
  printf("$ cnctty          : run the chat client.\n");

  exit(1);
}

int main(int argc, char *argv[])
{
  if (argc > 1)
  {
    if (argc == 2)
    {
      cnc_buffer *argument = cnc_buffer_init(1024);
      cnc_buffer_set_text(argument, argv[1]);

      if (cnc_buffer_equal_string(argument, "--version"))
      {
        printf("version: %s\n", APP_VERSION);
        cnc_buffer_destroy(argument);

        exit(0);
      }

      cnc_buffer_destroy(argument);
      usage();
    }

    usage();
  }

  int user_input                    = 0;
  bool end_app                      = false;
  bool display_disconnected_message = true;
  int net_connect_result            = 0;

  cnc_terminal *term         = NULL;
  cnc_net *net               = NULL;
  cnc_buffer *message_buffer = NULL;

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
  cnc_widget *prompt  = cnc_terminal_add_widget(term, WIDGET_PROMPT);

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
  cnc_buffer_set_text(display->data, "Welcome to cnctty ");
  cnc_buffer_append(display->data, APP_VERSION);

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

  net->databuffer     = display->data;
  net->message_buffer = message_buffer;
  net->terminal       = term;
  net->infobar        = infobar;
  net->username       = username;

  // user interface begins here
  // initially get the username
  cnc_buffer_append(display->data, "\n    Pick a username: _");
  cnc_terminal_update_and_redraw(term);

  while (true)
  {
    user_input = cnc_terminal_get_user_input(term);

    if (user_input == KEY_ENTER && prompt->data->length > 0)
    {
      cnc_buffer_set_text(username, prompt->data->contents);
      cnc_widget_reset(prompt);
      break;
    }
  }

  // now get the password
  cnc_buffer_delete_char(display->data, display->data->length - 1);
  cnc_buffer_append(display->data, username->contents);
  cnc_buffer_append(display->data, "\n    Password ? _");
  cnc_terminal_update_and_redraw(term);

  while (true)
  {
    user_input = cnc_terminal_get_user_input(term);

    if (user_input == KEY_ENTER)
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
  cnc_buffer_append(display->data,
                    password->length > 0 ? "yes..\n" : "no...\n");

  // select between user input and net receive
  struct timeval tv;
  fd_set read_fds;
  int net_fd, input_fd, nfds;
  int select_return_value;

  bool redraw_terminal;

  // loop while app has not ended
  while (!end_app)
  {
    redraw_terminal = false;

    // focus changed: refresh the infobar
    if (user_input == KEY_TAB)
    {
      set_info(infobar, net->connected ? "online" : "offline",
               display->has_focus
                   ? COLOR_YELLOW_BG
                   : (net->connected ? COLOR_GREEN_BG : COLOR_RED_BG),
               prompt, display);
      redraw_terminal = true;
    }

    // user wants to quit
    else if ((term->mode == MODE_INS && user_input == KEY_ENTER &&
              (cnc_buffer_equal_string(prompt->data, ":q") ||
               cnc_buffer_equal_string(prompt->data, ":Q"))) ||
             (term->mode == MODE_CMD &&
              (user_input == 'q' || user_input == 'Q')))
    {
      cnc_widget_reset(prompt);
      disconnect_from_server(net, infobar, term, display, prompt);
      end_app         = true;
      redraw_terminal = true;
    }

    // user wants to connect
    else if ((term->mode == MODE_INS && user_input == KEY_ENTER &&
              (cnc_buffer_equal_string(prompt->data, ":c") ||
               cnc_buffer_equal_string(prompt->data, ":C"))) ||
             (term->mode == MODE_CMD &&
              (user_input == 'c' || user_input == 'C')))
    {
      if (!net->connected)
      {
        cnc_widget_reset(prompt);
        set_info(infobar, "connecting...", COLOR_WHITE_BG, prompt, display);
        cnc_terminal_update_and_redraw(term);
        net_connect_result = cnc_net_connect(net);

        if (net_connect_result == 0)
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

          if (cnc_net_send(net, connect_string->contents) == -1)
          {
            cnc_net_disconnect(net);
            display_disconnected_message = true;
          }

          cnc_buffer_destroy(connect_string);
        }
      }

      redraw_terminal = true;
    }

    // user wants to disconnect from server
    else if ((term->mode == MODE_INS && user_input == KEY_ENTER &&
              (cnc_buffer_equal_string(prompt->data, ":d") ||
               cnc_buffer_equal_string(prompt->data, ":D"))) ||
             (term->mode == MODE_CMD &&
              (user_input == 'd' || user_input == 'd')))
    {
      cnc_widget_reset(prompt);
      disconnect_from_server(net, infobar, term, display, prompt);
      redraw_terminal = true;
    }

    // user wants to send message to the server
    else if (term->mode == MODE_INS && user_input == KEY_ENTER)
    {
      if (net->connected)
      {
        // changing username??
        if (prompt->data->contents[0] == '.' &&
            prompt->data->contents[1] == 'n')
        {
          cnc_buffer_clear(username);
          cnc_buffer_replace_text(username, 0, prompt->data->length - 2,
                                  prompt->data->contents, 2);
          cnc_buffer_trim(username);
        }

        if (cnc_net_send(net, prompt->data->contents) == -1)
        {
          cnc_net_disconnect(net);
          display_disconnected_message = true;
        }
      }

      cnc_widget_reset(prompt);
      redraw_terminal = true;
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

        redraw_terminal = true;
      }

      set_info(infobar, "offline",
               display->has_focus ? COLOR_YELLOW_BG : COLOR_RED_BG, prompt,
               display);
    }

    if (redraw_terminal)
    {
      cnc_terminal_update_and_redraw(term);
      redraw_terminal = false;
      user_input      = 0;
    }

    if (!end_app)
    {
      net_fd   = SSL_get_rfd(net->ssl);
      input_fd = fileno(stdin);
      nfds     = (net_fd > input_fd ? net_fd : input_fd) + 1;

      FD_ZERO(&read_fds);
      FD_SET(input_fd, &read_fds);

      if (net_fd > 0)
      {
        FD_SET(net_fd, &read_fds);
      }

      tv.tv_sec  = 1;
      tv.tv_usec = 0;

      // handle select error due to interrupted system call
      do
      {
        select_return_value = select(nfds, &read_fds, NULL, NULL, &tv);
      } while (select_return_value == -1 && errno == EINTR);

      if (select_return_value == -1)
      {
        free_all_allocations(term, net, username, password, message_buffer);

        printf("Select error!\n");
        perror("select()");
        exit(1);
      }

      else if (select_return_value == 0)
      {
        // check for terminal resize
        cnc_terminal_check_for_resize(term);

        // do not redraw
        redraw_terminal = false;
      }

      else
      {
        if (net->connected)
        {
          if (FD_ISSET(net_fd, &read_fds))
          {
            if (cnc_net_receive(net) < 0)
            {
              display_disconnected_message = true;
            }
          }
        }

        if (FD_ISSET(input_fd, &read_fds))
        {
          user_input = cnc_terminal_get_user_input(term);
        }
      }
    }
  }

  // eventually, ensure disconnection from server before exit
  disconnect_from_server(net, infobar, term, display, prompt);

  free_all_allocations(term, net, username, password, message_buffer);

  printf("No errors reported. See you next time!\n");

  return 0;
}
