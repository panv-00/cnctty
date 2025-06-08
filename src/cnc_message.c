#include "cnc_message.h"
#include "cnc_library/src/lib/cnc_buffer.h"

// private functions declarations
static void _cm_append_basic_message(cnc_buffer *buffer, cnc_buffer *message);
static void _cm_append_color_and_timestamp(cnc_buffer *buffer,
                                           const char *timestamp,
                                           uint32_t    color);
static void _cm_append_user_info(cnc_buffer *buffer, uint32_t user_id,
                                 cnc_buffer *from_to, const char *prefix);
static uint32_t _cm_extract_num(cnc_buffer *input, size_t start);

// private functions
static void _cm_append_basic_message(cnc_buffer *buffer, cnc_buffer *message)
{
  cb_append_buf(buffer, message);
}

static void _cm_append_color_and_timestamp(cnc_buffer *buffer,
                                           const char *timestamp,
                                           uint32_t    color)
{
  cb_push(buffer, CTT_PV(C_COL));
  cb_push(buffer, CTT_PV(color));
  if (timestamp)
  {
    cb_append_txt(buffer, timestamp);
    cb_push(buffer, CTT_PV(C_SPC));
  }
}

static void _cm_append_user_info(cnc_buffer *buffer, uint32_t user_id,
                                 cnc_buffer *from_to, const char *prefix)
{
  char buffer_id[11];
  sprintf(buffer_id, "%u", user_id);

  cb_append_txt(buffer, prefix);
  cb_append_txt(buffer, buffer_id);
  cb_append_txt(buffer, "]");
  cb_append_buf(buffer, from_to);
  cb_append_txt(buffer, ":");
}

static uint32_t _cm_extract_num(cnc_buffer *input, size_t start)
{
  if (input == NULL || input->data == NULL)
  {
    return 0;
  }

  uint32_t output = 0;
  uint32_t value  = 0;
  size_t   index  = start;

  while (index < input->size)
  {
    value = input->data[index++].token.value;

    if (value < '0' || value > '9')
    {
      return output;
    }

    output = 10 * output + value - '0';
  }

  return output;
}

// end of private functions

void cm_add_buffer_to_messages(const char *buffer, size_t bytes_received,
                               cnc_buffer *msg_buffer, cnc_buffer *username,
                               cnc_app *app)
{
  if (buffer == NULL || app == NULL || msg_buffer == NULL ||
      msg_buffer->data == NULL || username == NULL || username->data == NULL)
  {
    return;
  }

  if (bytes_received <= 0)
  {
    return;
  }

  cnc_term_token tkn   = {0};
  size_t         index = 0;

  while (index < bytes_received)
  {
    if (buffer[index] == C_NUL || buffer[index] == C_RET)
    {
      index++;
    }

    else if (buffer[index] == C_ENT)
    {
      // cb_push(message_buffer, CTT_PV(0));
      cm_parse(msg_buffer, username, app);
      ca_update(app);
      cb_clear(msg_buffer);
      index++;
    }

    else
    {
      ctt_parse_bytes((uint8_t *)&buffer[index], &tkn);
      cb_push(msg_buffer, tkn);
      index += tkn.token.length;
    }
  }
}

bool cm_init(cnc_message *cm)
{
  if (cm == NULL)
  {
    return false;
  }

  if (cb_init(&(cm->from_to), MAX_NAME) == false)
  {
    return false;
  }

  if (cb_init(&(cm->body), MAX_MESSAGE_BODY) == false)
  {
    cb_destroy(&(cm->from_to));

    return false;
  }

  cm->type    = CM_NONE;
  cm->user_id = 0;

  return true;
}

void cm_destroy(cnc_message *cm)
{
  if (cm == NULL)
  {
    return;
  }

  cb_destroy(&cm->from_to);
  cb_destroy(&cm->body);
}

void cm_parse(cnc_buffer *msg_buffer, cnc_buffer *username, cnc_app *app)
{
  if (app == NULL || msg_buffer == NULL || msg_buffer->data == NULL ||
      username == NULL || username->data == NULL)
  {
    return;
  }

  size_t      start_index;
  size_t      stop_index;
  time_t      t;
  struct tm  *tm_info;
  char        timestamp[6];
  cnc_message this_message;

  if (cm_init(&this_message) == false)
  {
    return;
  }

  time(&t);
  tm_info = localtime(&t);

  strftime(timestamp, sizeof(timestamp), "%H:%M", tm_info);
  timestamp[5] = C_NUL;

  // private message received
  if (msg_buffer->data[0].token.value == '<')
  {
    if (cb_locate_c_str(msg_buffer, ">", &start_index) &&
        cb_locate_c_str(msg_buffer, " (private):", &stop_index))
    {
      this_message.type    = CM_PRIV_IN;
      this_message.user_id = _cm_extract_num(msg_buffer, 1);

      for (size_t i = start_index + 1; i < stop_index; i++)
      {
        if (this_message.from_to.size < MAX_NAME)
        {
          cb_push(&this_message.from_to, msg_buffer->data[i]);
        }
      }

      for (size_t i = stop_index + 12; i < msg_buffer->size; i++)
      {
        if (this_message.body.size < MAX_MESSAGE_BODY)
        {
          cb_push(&this_message.body, msg_buffer->data[i]);
        }
      }
    }

    else
    {
      // badly constructed message. send it as is
      // cb_set_txt(&this_message.body, "[ ... ]");
      cb_set_buf(&this_message.body, msg_buffer);
    }
  } // end private message received

  // private message sent
  else if (cb_locate_c_str(msg_buffer, ">> Message sent to [", &start_index))
  {
    this_message.user_id = _cm_extract_num(msg_buffer, start_index + 20);

    if (cb_locate_c_str(msg_buffer, "]", &start_index) &&
        cb_locate_c_str(msg_buffer, ":", &stop_index))
    {
      for (size_t i = start_index + 1; i < stop_index; i++)
      {
        if (this_message.from_to.size < MAX_NAME)
        {
          cb_push(&this_message.from_to, msg_buffer->data[i]);
        }
      }

      // now get the actual message after "(private):"
      if (cb_locate_c_str(msg_buffer, "(private):", &start_index))
      {
        this_message.type = CM_PRIV_OUT;

        for (size_t i = start_index + 10; i < msg_buffer->size; i++)
        {
          if (this_message.body.size < MAX_MESSAGE_BODY)
          {
            cb_push(&this_message.body, msg_buffer->data[i]);
          }
        }
      }

      else
      {
        // badly constructed message. send it as is
        // cb_set_txt(&this_message.body, "[ ... ]");
        cb_set_buf(&this_message.body, msg_buffer);
      }
    }

    else
    {
      // badly constructed message. send it as is
      // cb_set_txt(&this_message.body, "[ ... ]");
      cb_set_buf(&this_message.body, msg_buffer);
    }
  } // end private message sent

  // message emote
  else if (msg_buffer->data[0].token.value == '(')
  {
    this_message.type = CM_EMOTE;
    cb_append_buf(&this_message.body, msg_buffer);
  } // end message emote

  // message sent or received
  else if (msg_buffer->data[0].token.value == '[')
  {
    this_message.user_id = _cm_extract_num(msg_buffer, 1);

    if (cb_locate_c_str(msg_buffer, "]", &start_index) &&
        cb_locate_c_str(msg_buffer, ":", &stop_index))
    {
      for (size_t i = start_index + 1; i < stop_index; i++)
      {
        if (this_message.from_to.size < MAX_NAME)
        {
          cb_push(&this_message.from_to, msg_buffer->data[i]);
        }
      }

      if (cb_equal(&this_message.from_to, username))
      {
        this_message.type = CM_SENT;
      }

      else
      {
        this_message.type = CM_RECEIVED;
      }

      for (size_t i = stop_index + 1; i < msg_buffer->size; i++)
      {
        if (this_message.body.size < MAX_MESSAGE_BODY)
        {
          cb_push(&this_message.body, msg_buffer->data[i]);
        }
      }
    }

    else
    {
      // badly constructed message. send it as is
      // cb_set_txt(&this_message.body, "[ ... ]");
      cb_set_buf(&this_message.body, msg_buffer);
    }
  } // end message sent or received

  // system message
  else if (msg_buffer->data[0].token.value == '>' &&
           msg_buffer->data[1].token.value == '>')
  {
    this_message.type = CM_SYSTEM;
    cb_append_buf(&this_message.body, msg_buffer);
  } // end system message

  // something else
  else
  {
    cb_append_buf(&this_message.body, msg_buffer);
  }

  cnc_buffer formatted_message;
  cb_init(&formatted_message, MAX_MESSAGE_BODY);

  switch (this_message.type)
  {
    case CM_SYSTEM:
      _cm_append_color_and_timestamp(&formatted_message, NULL, KS_RED_FG);
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_EMOTE:
      _cm_append_color_and_timestamp(&formatted_message, timestamp, KS_GRE_FG);
      cb_append_txt(&formatted_message, "~~ ");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_SENT:
      _cm_append_color_and_timestamp(&formatted_message, timestamp, KS_CYA_FG);
      _cm_append_user_info(&formatted_message, this_message.user_id,
                           &this_message.from_to, ">> [");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_PRIV_OUT:
      _cm_append_color_and_timestamp(&formatted_message, timestamp, KS_MAG_FG);
      _cm_append_user_info(&formatted_message, this_message.user_id,
                           &this_message.from_to, ">> [");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_PRIV_IN:
      _cm_append_color_and_timestamp(&formatted_message, timestamp, KS_YEL_FG);
      _cm_append_user_info(&formatted_message, this_message.user_id,
                           &this_message.from_to, "<< [");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_RECEIVED:
      cb_append_txt(&formatted_message, timestamp);
      cb_push(&formatted_message, CTT_PV(C_SPC));
      _cm_append_user_info(&formatted_message, this_message.user_id,
                           &this_message.from_to, "<< [");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;

    case CM_NONE:
    default:
      cb_append_txt(&formatted_message, "| ");
      _cm_append_basic_message(&formatted_message, &this_message.body);
      break;
  }

  // set the index of first message to be displayed
  // calculate this message number of rows to add to previous data_index
  size_t rows             = 0;
  size_t counter          = 0;
  size_t last_space_index = 0;
  size_t first_index      = 0;
  size_t row_width        = 0;

  // calculation phase
  while (counter < formatted_message.size)
  {
    row_width =
      cb_data_width(&formatted_message, first_index, counter - first_index + 1);

    if (row_width > app->cterm->scr_cols)
    {
      rows++;

      if (last_space_index > first_index)
      {
        counter = last_space_index + 1;

        // Skip any additional whitespace after wrap
        while (counter < formatted_message.size &&
               ctt_is_whitespace(formatted_message.data[counter]))
        {
          counter++;
        }
      }

      first_index = counter;
      continue;
    }

    if (ctt_is_whitespace(formatted_message.data[counter]))
    {
      last_space_index = counter++;
      continue;
    }

    // Colors decryption
    if (formatted_message.data[counter].token.value == C_COL)
    {
      counter += 2;
      continue;
    }

    counter++;
  }

  // Finally, end the message
  cb_push(&formatted_message, CTT_PV(C_ENT));
  cb_append_buf(&app->cw_display->buffer, &formatted_message);
  cm_destroy(&this_message);
  cb_destroy(&formatted_message);

  if (app->cw_display->data_index + rows > app->cw_display->frame.height)
  {
    app->cw_display->index =
      app->cw_display->data_index - app->cw_display->frame.height + rows + 1;
  }
}
