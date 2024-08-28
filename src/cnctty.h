#ifndef CNC_TTY_H
#define CNC_TTY_H

#include "cnc_library/cnc_library.h"
#include "cnc_message.h"
#include "cnc_net.h"

#define APP_VERSION "0.1.33"

#define TERM_MIN_WIDTH 68
#define TERM_MIN_HEIGHT 15

void set_info(cnc_widget *i, const char *text, char *bg, cnc_widget *p,
              cnc_widget *d);

int connect_to_server(cnc_net *n);
void disconnect_from_server(cnc_net *net, cnc_widget *infobar,
                            cnc_terminal *term, cnc_widget *display,
                            cnc_widget *prompt);

void free_all_allocations(cnc_terminal *term, cnc_net *net,
                          cnc_buffer *username, cnc_buffer *password,
                          cnc_buffer *message_buffer);

int main(void);

#endif
