#ifndef CNC_MESSAGE_H
#define CNC_MESSAGE_H

#define MAX_MESSAGE_BODY 542
#define MAX_NAME 17

#include <stddef.h>
#include <time.h>

#include "cnc_library/cnc_library.h"

void add_buffer_to_messages(const char *buffer, size_t bytes_received,
                            cnc_buffer *message_buffer, const char *username,
                            cnc_buffer *databuffer, cnc_terminal *term,
                            cnc_widget *infobar);

void message_parse(const char *message_string, size_t length,
                   const char *username, cnc_buffer *databuffer,
                   cnc_terminal *term, cnc_widget *infobar);

#endif
