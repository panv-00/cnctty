#include "cnc_message.h"

void add_buffer_to_messages(const char *buffer, size_t bytes_received,
                            cnc_buffer *message_buffer, const char *username,
                            cnc_buffer *databuffer, cnc_terminal *term,
                            cnc_widget *infobar)
{
  if (bytes_received <= 0)
  {
    return;
  }

  for (int i = 0; i < bytes_received; i++)
  {
    if (buffer[i] == '\0' || buffer[i] == '\r')
    {
      continue;
    }

    if (buffer[i] == '\n')
    {
      cnc_buffer_insert_char(message_buffer, message_buffer->length, 1, '\0');
      message_parse(message_buffer->contents, message_buffer->length, username,
                    databuffer, term, infobar);
      cnc_terminal_update_and_redraw(term);
      cnc_buffer_clear(message_buffer);
    }

    else
    {
      cnc_buffer_insert_char(message_buffer, message_buffer->length, 1,
                             buffer[i]);
    }
  }
}

void message_parse(const char *message_string, size_t length,
                   const char *username, cnc_buffer *databuffer,
                   cnc_terminal *term, cnc_widget *infobar)
{
  if (databuffer->length + MAX_MESSAGE_BODY > databuffer->size)
  {
    // before parsing any message, we need to be sure that the databuffer
    // can contain an additional message.
    // if not, we need to empty old messages from it, so that at least
    // 50% will be free for new messages.

    size_t start_index = databuffer->length - databuffer->size / 2;

    while (databuffer->contents[start_index] != '\n')
    {
      // loop till the first beginning of a line
      start_index++;
    }

    start_index++;

    for (size_t i = start_index; i < databuffer->length; i++)
    {
      databuffer->contents[i - start_index] = databuffer->contents[i];
    }

    for (size_t i = databuffer->length - start_index; i < databuffer->size; i++)
    {
      databuffer->contents[i] = '\0';
    }

    databuffer->length -= start_index;
    cnc_buffer_append(databuffer, "\n");
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_MAGENTA_FG);
    cnc_buffer_append(databuffer, "  ** cleared few old messages **\n");
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);
    cnc_buffer_append(databuffer, "\n");
  }

  size_t tmp_index;
  time_t t;
  struct tm *tm_info;
  char timestamp[9];
  cnc_buffer *tmp_buf = cnc_buffer_init(MAX_MESSAGE_BODY);
  cnc_buffer *msg_buf = cnc_buffer_init(MAX_MESSAGE_BODY);

  cnc_buffer_set_text(msg_buf, message_string);

  time(&t);
  tm_info = localtime(&t);

  strftime(timestamp, 9, "%H:%M:%S", tm_info);
  timestamp[8] = '\0';

  if (message_string[0] == '<')
  {
    // private message received
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_MAGENTA_BG);

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_BLACK_FG);

    // insert timestamp
    cnc_buffer_append(databuffer, timestamp);
    cnc_buffer_append(databuffer, " ");

    if (cnc_buffer_locate_string(msg_buf, " (private):", &tmp_index))
    {
      // add received sign (FR: )
      cnc_buffer_append(databuffer, "FR ");

      // message from -> tmp_buffer
      cnc_buffer_replace_text(tmp_buf, 0, tmp_index, msg_buf->contents, 0);
      cnc_buffer_append(databuffer, tmp_buf->contents);
      cnc_buffer_append(databuffer, ": ");
      cnc_buffer_clear(tmp_buf);

      // message body -> tmp_buffer
      cnc_buffer_replace_text(tmp_buf, 0, msg_buf->length - tmp_index - 12,
                              msg_buf->contents, tmp_index + 12);
      cnc_buffer_append(databuffer, tmp_buf->contents);
      cnc_buffer_clear(tmp_buf);
    }

    else
    {
      cnc_buffer_append(databuffer, "[ ... ]");
    }

    cnc_buffer_append(databuffer, "\n");
  }

  else if (cnc_buffer_locate_string(msg_buf, ">> Message sent to", &tmp_index))
  {
    // private message sent
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_MAGENTA_FG);

    // insert timestamp
    cnc_buffer_append(databuffer, timestamp);
    cnc_buffer_append(databuffer, " ");

    size_t start, end;

    if (cnc_buffer_locate_string(msg_buf, "[", &start))
    {
      if (cnc_buffer_locate_string(msg_buf, ":", &end))
      {
        // add sent sign (TO: )
        cnc_buffer_append(databuffer, "TO ");

        // user to -> tmp_buffer
        cnc_buffer_replace_text(tmp_buf, 0, end - start, msg_buf->contents,
                                start);
        cnc_buffer_append(databuffer, tmp_buf->contents);
        cnc_buffer_append(databuffer, ": ");
        cnc_buffer_clear(tmp_buf);
      }
    }

    else
    {
      cnc_buffer_append(databuffer, "[ ... ]");
    }

    if (cnc_buffer_locate_string(msg_buf, " (private):", &tmp_index))
    {
      // message body -> tmp_buffer
      cnc_buffer_replace_text(tmp_buf, 0, msg_buf->length - tmp_index - 12,
                              msg_buf->contents, tmp_index + 12);
      cnc_buffer_append(databuffer, tmp_buf->contents);
      cnc_buffer_clear(tmp_buf);
    }

    else
    {
      cnc_buffer_append(databuffer, "[ ... ]");
    }

    cnc_buffer_append(databuffer, "\n");
  }

  else if (message_string[0] == '(')
  {
    // message emote
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_GREEN_FG);

    // insert timestamp
    cnc_buffer_append(databuffer, timestamp);
    cnc_buffer_append(databuffer, " ");

    cnc_buffer_append(databuffer, msg_buf->contents);
    cnc_buffer_append(databuffer, "\n");
  }

  else if (message_string[0] == '[')
  {
    // message can be either sent, or received.
    // we need to compare the username...

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);

    if (cnc_buffer_locate_string(msg_buf, "]", &tmp_index))
    {
      cnc_buffer *uname = cnc_buffer_init(MAX_NAME);
      size_t end;

      if (cnc_buffer_locate_string(msg_buf, ":", &end))
      {
        // message from -> tmp_buffer
        cnc_buffer_replace_text(tmp_buf, 0, end, msg_buf->contents, 0);

        // stripped message from -> uname
        cnc_buffer_replace_text(uname, 0, end - tmp_index - 1,
                                msg_buf->contents, tmp_index + 1);

        if (cnc_buffer_equal_string(uname, username))
        {
          // message is sent: set the sent color indicator
          cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                                 COLOR_INFO_BYTE);
          cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                                 COLOR_CODE_GREEN_FG);
        }

        // insert timestamp
        cnc_buffer_append(databuffer, timestamp);
        cnc_buffer_append(databuffer, " ");

        cnc_buffer_append(databuffer, tmp_buf->contents);
        cnc_buffer_append(databuffer, ": ");

        cnc_buffer_clear(tmp_buf);

        // message body -> tmp_buffer
        cnc_buffer_replace_text(tmp_buf, 0, msg_buf->length - end - 2,
                                msg_buf->contents, end + 2);
        cnc_buffer_append(databuffer, tmp_buf->contents);
        cnc_buffer_clear(tmp_buf);
      }

      else
      {
        // message is neither sent nor received... print it as is...

        cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                               COLOR_INFO_BYTE);
        cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                               COLOR_CODE_DEFAULT);

        cnc_buffer_append(databuffer, msg_buf->contents);
      }

      // destroy uname
      cnc_buffer_destroy(uname);
    }

    else
    {
      cnc_buffer_append(databuffer, "[ ... ]");
    }

    cnc_buffer_append(databuffer, "\n");
  }

  else if (message_string[0] == '>' && message_string[1] == '>')
  {
    // system message

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);

    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_RED_FG);

    cnc_buffer_append(databuffer, msg_buf->contents);
    cnc_buffer_append(databuffer, "\n");
  }

  else
  {
    // message none
    cnc_buffer_insert_char(databuffer, databuffer->length, 1, COLOR_INFO_BYTE);
    cnc_buffer_insert_char(databuffer, databuffer->length, 1,
                           COLOR_CODE_DEFAULT);

    cnc_buffer_append(databuffer, msg_buf->contents);
    cnc_buffer_append(databuffer, "\n");
  }

  char buf[14];
  sprintf(buf, "[%5zu/%5zu]", databuffer->length, databuffer->size);
  cnc_buffer_replace_text(infobar->data, 0, 14, buf, 0);

  cnc_buffer_destroy(msg_buf);
  cnc_buffer_destroy(tmp_buf);
}
