#include "cnctty.h"
#include "cnc_message.h"
#include "cnc_net.h"

void usage(int exit_code)
{
  printf("Usage:\n");
  printf("$ cnctty --help   : display this help message.\n");
  printf("$ cnctty --version: display version and exit.\n");
  printf("$ cnctty          : run the chat client.\n");

  exit(exit_code);
}

int main(int argc, char *argv[])
{
  if (argc > 1)
  {
    if (argc == 2)
    {
      cnc_buffer argument;
      cb_init(&argument, 1024);
      cb_set_txt(&argument, argv[1]);

      if (cb_equal_c_str(&argument, "--version"))
      {
        printf("version: %s\n", APP_VERSION);
        cb_destroy(&argument);

        exit(0);
      }

      if (cb_equal_c_str(&argument, "--help"))
      {
        cb_destroy(&argument);
        usage(0);
      }

      cb_destroy(&argument);
      usage(1);
    }

    usage(1);
  }

  int  user_input       = 0;
  bool end_app          = false;
  bool getting_username = true;
  bool getting_password = false;
  bool retry_username   = false;
  bool redraw_terminal  = false;

  cnc_app cnctty;

  if (ca_init(&cnctty, MIN_TERM_HEIGHT, MIN_TERM_WIDTH) == false)
  {
    ca_destroy(&cnctty);

    printf("Couldn't initialize app. Exiting..\n");
    exit(1);
  }

  if (ca_setup(&cnctty, APP_VERSION, " CNCTTY", "Username: _", "offline") ==
      false)
  {
    ca_destroy(&cnctty);

    printf("Couldn't setup the terminal. Exiting..\n");
    exit(1);
  }

  // assign pointers for ease of use
  cnc_terminal *term = cnctty.cterm;
  // cnc_widget   *title   = cnctty.cw_title_bar;
  cnc_widget *display = cnctty.cw_display;
  cnc_widget *info    = cnctty.cw_info_bar;
  cnc_widget *prompt  = cnctty.cw_prompt;

  // set main display widget
  term->main_display_widget = display;

  // initially, app is offline. set info colors
  info->bg = CTT_PV(KS_RED_BG);
  info->fg = CTT_PV(KS_BLA_FG);

  term->can_change_focus = true;
  term->can_change_mode  = true;

  // uptade app before run loop
  ct_update(cnctty.cterm);

  // prepare variables
  cnc_net *net = cn_init();

  if (net == NULL)
  {
    ca_destroy(&cnctty);

    printf("Couldn't initialize network. Exiting..\n");
    exit(1);
  }

  cnc_buffer username;
  cnc_buffer password;
  cnc_buffer msg_buffer;

  if (cb_init(&username, MAX_NAME) == false ||
      cb_init(&password, MAX_NAME) == false ||
      cb_init(&msg_buffer, MAX_MESSAGE_BODY) == false)
  {
    cn_destroy(&net);
    ca_destroy(&cnctty);

    printf("Couldn't initialize app buffers..\n");
    exit(1);
  }

  // assign network pointers
  net->username   = &username;
  net->msg_buffer = &msg_buffer;
  net->app        = &cnctty;

  // select variables
  struct timeval tv;

  fd_set read_fds;
  int    net_fd, input_fd, nfds;
  int    select_return_value;

  // loop while app has not ended
  while (!end_app)
  {
    redraw_terminal = true;

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
      ca_set_info(&cnctty, "quitting...");
      ca_update(&cnctty);

      cb_destroy(&username);
      cb_destroy(&password);
      cb_destroy(&msg_buffer);
      cn_destroy(&net);
      ca_destroy(&cnctty);

      printf("Select error!\n");
      perror("select()");
      exit(1);
    }

    else if (select_return_value == 0)
    {
      // check for terminal resize
      // exposed function for faster response to resize
      ct_check_for_resize(term);
      redraw_terminal = false;
    }

    else
    {
      if (net->connected)
      {
        if (FD_ISSET(net_fd, &read_fds))
        {
          if (cn_receive(net) < 0)
          {
            cn_disconnect(net);
          }
        }
      }

      if (FD_ISSET(input_fd, &read_fds))
      {
        user_input = ca_get_user_input(&cnctty);
      }
    }

    if (user_input == C_ENT)
    {
      // reset user_input to avoid wrong parse while waiting for input
      user_input = 0;

      // App is just starting? user has to pick the username
      // then the password
      if (getting_username)
      {
        cb_set_buf(&username, &prompt->buffer);

        cb_remove(&display->buffer, display->buffer.size - 1);
        cb_append_buf(&display->buffer, &username);
        cb_append_txt(&display->buffer, "\nPassword? ");

        getting_username = false;
        getting_password = true;
      }

      else if (getting_password)
      {
        cb_set_buf(&password, &prompt->buffer);

        if (password.size > 0)
        {
          cb_append_txt(&display->buffer, "***");
        }

        else
        {
          cb_append_txt(&display->buffer, "No Password");
        }

        cb_append_txt(&display->buffer, "\n\nSystem main commands:");
        cb_push(&display->buffer, CTT_PV(C_COL));
        cb_push(&display->buffer, CTT_PV(KS_YEL_FG));

        cb_append_txt(&display->buffer,
                      "\n  >> :r -> reset username (startup only)");
        cb_push(&display->buffer, CTT_PV(C_COL));
        cb_push(&display->buffer, CTT_PV(KS_YEL_FG));

        cb_append_txt(&display->buffer, "\n  >> :q -> quit");
        cb_push(&display->buffer, CTT_PV(C_COL));
        cb_push(&display->buffer, CTT_PV(KS_YEL_FG));

        cb_append_txt(&display->buffer, "\n  >> :c -> connect");
        cb_push(&display->buffer, CTT_PV(C_COL));
        cb_push(&display->buffer, CTT_PV(KS_YEL_FG));

        cb_append_txt(&display->buffer, "\n  >> :d -> disconnect");
        cb_push(&display->buffer, CTT_PV(C_COL));
        cb_push(&display->buffer, CTT_PV(KS_YEL_FG));
        cb_append_txt(&display->buffer, "\n\n");

        getting_password = false;
        retry_username   = true;
      }

      // user want to repick his username
      else if (retry_username && cb_equal_c_str(&prompt->buffer, ":r"))
      {
        retry_username = false;
        cb_set_txt(&display->buffer, "Username: _");
        getting_username = true;
      }

      // user wants to quit
      else if (cb_equal_c_str(&prompt->buffer, ":q"))
      {
        end_app = true;
      }

      // user wants to connect
      else if (cb_equal_c_str(&prompt->buffer, ":c"))
      {
        // no more changing name with :r after connection
        retry_username = false;

        cw_reset(prompt);

        if (net->connected == false)
        {
          info->bg = CTT_PV(KS_YEL_BG);
          info->fg = CTT_PV(KS_BLA_FG);

          ca_set_info(&cnctty, "connecting...");
          ca_update(&cnctty);
          sleep(1);

          if (cn_connect(net))
          {
            cnc_buffer connect_string;
            cb_init(&connect_string, 2 * MAX_NAME + 4);
            cb_set_txt(&connect_string, ".n ");
            cb_append_buf(&connect_string, &username);

            if (password.size > 0)
            {
              cb_append_txt(&connect_string, "=");
              cb_append_buf(&connect_string, &password);
            }

            char connect_c_str[2 * MAX_NAME + 5];
            cb_set_c_str(&connect_string, connect_c_str, 2 * MAX_NAME + 4);

            if (cn_send(net, connect_c_str) <= 0)
            {
              cn_disconnect(net);
            }

            cb_destroy(&connect_string);
          }
        }
      }

      // user wants to disconnect
      else if (cb_equal_c_str(&prompt->buffer, ":d"))
      {
        // clear the prompt without delay
        cw_reset(prompt);

        info->bg = CTT_PV(KS_YEL_BG);
        info->fg = CTT_PV(KS_BLA_FG);

        ca_set_info(&cnctty, "disconnecting...");
        ca_update(&cnctty);

        cn_disconnect(net);
      }

      // user is sending message to the server
      else if (net->connected)
      {
        // changing username??
        if (prompt->buffer.data[0].token.value == '.' && //
            prompt->buffer.data[1].token.value == 'n')   // .n ???
        {
          size_t equal_location = 0;

          if (cb_locate_c_str(&prompt->buffer, "=", &equal_location))
          {
            cb_clear(&username);

            for (size_t i = 2; i < equal_location; i++)
            {
              // ignore spaces at the beginning
              if (username.size == 0 &&
                  prompt->buffer.data[i].token.value == C_SPC)
              {
                continue;
              }

              cb_push(&username, prompt->buffer.data[i]);
            }
          }

          else
          {
            cb_clear(&username);

            for (size_t i = 2; i < prompt->buffer.size; i++)
            {
              // ignore spaces at the beginning
              if (username.size == 0 &&
                  prompt->buffer.data[i].token.value == C_SPC)
              {
                continue;
              }

              cb_push(&username, prompt->buffer.data[i]);
            }
          }
        }

        char message[PROMPT_BUFFER_SIZE + 1];
        cb_set_c_str(&prompt->buffer, message, sizeof(message));

        if (cn_send(net, message) <= 0)
        {
          cn_disconnect(net);
        }
      }

      cw_reset(prompt);
    }

    if (net->connected)
    {
      // back to default colors
      info->bg = CTT_PV(KS_NON___);
      info->fg = CTT_PV(KS_NON___);

      ca_set_info(&cnctty, "online");
    }

    else
    {
      info->bg = CTT_PV(KS_RED_BG);
      info->fg = CTT_PV(KS_BLA_FG);

      ca_set_info(&cnctty, "offline");
    }

    if (redraw_terminal)
    {
      ca_update(&cnctty);
    }
  }

  info->bg = CTT_PV(KS_YEL_BG);
  info->fg = CTT_PV(KS_BLA_FG);

  ca_set_info(&cnctty, "quitting...");
  ca_update(&cnctty);

  cb_destroy(&username);
  cb_destroy(&password);
  cb_destroy(&msg_buffer);
  cn_destroy(&net);
  ca_destroy(&cnctty);

  printf("Thank you for trying cnctty. No errors reported!\n");

  return 0;
}
