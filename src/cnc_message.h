#ifndef CNC_MESSAGE_H
#define CNC_MESSAGE_H

#define MAX_MESSAGE_BODY 600
#define MAX_NAME         17

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "cnc_library/src/lib/cnc_library.h"

// we will use cm as prefix for cnc_message

typedef enum
{
  CM_NONE = 0,
  CM_SYSTEM,
  CM_EMOTE,
  CM_SENT,
  CM_RECEIVED,
  CM_PRIV_OUT,
  CM_PRIV_IN,

} cm_type;

typedef struct
{
  cm_type    type;
  uint32_t   user_id;
  cnc_buffer from_to;
  cnc_buffer body;

} cnc_message;

void cm_add_buffer_to_messages(const char *buffer, size_t bytes_received,
                               cnc_buffer *msg_buffer, cnc_buffer *username,
                               cnc_app *app);

bool cm_init(cnc_message *cm);
void cm_destroy(cnc_message *cm);

void cm_parse(cnc_buffer *msg_buffer, cnc_buffer *username, cnc_app *app);
void cm_parse_test(cnc_buffer *msg_buffer, cnc_buffer *username);

#endif
