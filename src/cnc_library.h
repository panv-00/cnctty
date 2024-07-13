#ifndef CNC_LIBRARY_H
#define CNC_LIBRARY_H

/*** 0. GENERAL ***/
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// define empty character using UNIT SEPARATOR to align char array
// may not be used implicitly
#define USC 31 // '\x1F'

// max buffers sizes
#define INFO_BUFFER_SIZE 511
#define PROMPT_BUFFER_SIZE 511
#define DISPLAY_BUFFER_SIZE 99999

/*** 1. COLORS ***/
// each color define has a size of 5 bytes, except COLOR_DEFAULT is 4 bytes
#define COLOR_NONE "\x1f\x1f\x1f\x1f\x1f"
#define COLOR_DEFAULT "\x1b[0m" // 4 bytes
#define COLOR_BLACK_FG "\x1b[30m"
#define COLOR_RED_FG "\x1b[31m"
#define COLOR_GREEN_FG "\x1b[32m"
#define COLOR_YELLOW_FG "\x1b[33m"
#define COLOR_BLUE_FG "\x1b[34m"
#define COLOR_MAGENTA_FG "\x1b[35m"
#define COLOR_CYAN_FG "\x1b[36m"
#define COLOR_WHITE_FG "\x1b[37m"
#define COLOR_BLACK_BG "\x1b[40m"
#define COLOR_RED_BG "\x1b[41m"
#define COLOR_GREEN_BG "\x1b[42m"
#define COLOR_YELLOW_BG "\x1b[43m"
#define COLOR_BLUE_BG "\x1b[44m"
#define COLOR_MAGENTA_BG "\x1b[45m"
#define COLOR_CYAN_BG "\x1b[46m"
#define COLOR_WHITE_BG "\x1b[47m"

#define COLOR_INFO_BYTE 5
#define COLOR_CODE_NONE 97
#define COLOR_CODE_DEFAULT 98
#define COLOR_CODE_BLACK_FG 99
#define COLOR_CODE_RED_FG 100
#define COLOR_CODE_GREEN_FG 101
#define COLOR_CODE_YELLOW_FG 102
#define COLOR_CODE_BLUE_FG 103
#define COLOR_CODE_MAGENTA_FG 104
#define COLOR_CODE_CYAN_FG 105
#define COLOR_CODE_WHITE_FG 106
#define COLOR_CODE_BLACK_BG 107
#define COLOR_CODE_RED_BG 108
#define COLOR_CODE_GREEN_BG 109
#define COLOR_CODE_YELLOW_BG 110
#define COLOR_CODE_BLUE_BG 111
#define COLOR_CODE_MAGENTA_BG 112
#define COLOR_CODE_CYAN_BG 113
#define COLOR_CODE_WHITE_BG 114

// following function returns 1 for bg color and 2 for fg color
// it ultimitely sets the color in char *color
// this function will return 0 when resetting colors
int _color_code_to_color(int color_code, char **color);

/*** 2. BUFFER ***/
typedef struct
{
  size_t length;
  size_t size;
  char *contents;

} cnc_buffer;

size_t calen(const char *str);

cnc_buffer *cnc_buffer_init(size_t size);
cnc_buffer *cnc_buffer_resize(cnc_buffer *b, size_t size);
void cnc_buffer_set_text(cnc_buffer *b, const char *text);
void cnc_buffer_replace(cnc_buffer *b, const char orig, const char dest);
void cnc_buffer_replace_text(cnc_buffer *b, size_t start, size_t len,
                             const char *text, size_t text_start);
void cnc_buffer_replace_char(cnc_buffer *b, size_t start, size_t len, char c);
size_t cnc_buffer_insert_text(cnc_buffer *b, size_t start, size_t len,
                              const char *text);
size_t cnc_buffer_append(cnc_buffer *b, const char *text);
size_t cnc_buffer_insert_char(cnc_buffer *b, size_t start, size_t len, char c);
bool cnc_buffer_delete_char(cnc_buffer *b, size_t location);
bool cnc_buffer_equal_string(cnc_buffer *b, const char *str);
bool cnc_buffer_locate_string(cnc_buffer *b, const char *s, size_t *location);
void cnc_buffer_trim(cnc_buffer *b);
void cnc_buffer_clear(cnc_buffer *b);
void cnc_buffer_destroy(cnc_buffer *b);

/*** 3. WIDGETS ***/

// prompt symbols
#define PROMPT_INS "$ " // 2 bytes
#define PROMPT_CMD ": " // 2 bytes
#define PROMPT_NUL "  " // 2 bytes

typedef enum
{
  WIDGET_INFO = 1,
  WIDGET_PROMPT,
  WIDGET_DISPLAY

} cnc_widget_type;

typedef struct
{
  size_t row;
  size_t col;

} cnc_point;

typedef struct
{
  cnc_point origin;
  size_t width;
  size_t height;

} cnc_rect;

typedef struct
{
  cnc_rect frame;
  cnc_widget_type type;

  // index: index of first visible element
  size_t index;

  // data_index: index of cursor in the data
  size_t data_index;

  cnc_buffer *data;

  // info and prompt have homogeneous bg and fg colors
  char *background;
  char *foreground;

  bool can_focus;
  bool has_focus;

} cnc_widget;

cnc_widget *cnc_widget_init(cnc_widget_type type);
void cnc_widget_reset(cnc_widget *w);
void cnc_widget_destroy(cnc_widget *w);

/*** 4. TERMINAL ***/

// Cursor and Screen Stuff
#define CLRSCR write(STDOUT_FILENO, "\x1b[2J\x1b[3J", 8)
#define HOME_POSITION write(STDOUT_FILENO, "\x1b[H", 3)
#define HIDE_CURSOR write(STDOUT_FILENO, "\x1b[?25l", 6)
#define SHOW_CURSOR write(STDOUT_FILENO, "\x1b[?25h", 6)
#define CURSOR_INS write(STDOUT_FILENO, "\x1b[5 q", 5)
#define CURSOR_CMD write(STDOUT_FILENO, "\x1b[1 q", 5)

// Keyboard
// CTRL-KEY Combination
#define CTRL_KEY(k) ((k) & 0x1f)

// Escape Characters
#define TAB 9
#define ESCAPE 27
#define ARROW 91
#define ARROW_UP 65
#define ARROW_DN 66
#define ARROW_RT 67
#define ARROW_LT 68

// Custom Escape Characters
#define TAB_KEY 1009
#define ESCAPE_KEY 1027
#define ARROW_KEY 1091
#define ARROW_UP_KEY 1065
#define ARROW_DN_KEY 1066
#define ARROW_RT_KEY 1067
#define ARROW_LT_KEY 1068
#define BACKSPACE_KEY 127
#define ENTER_KEY 10

// Terminal Size Error
#define TERM_TOO_SMALL -1

// Position Cursor Function
void POSCURSOR(size_t c, size_t r);

// Vim-Like functions
void _VimMode__k(cnc_widget *w);
void _VimMode__j(cnc_widget *w);
void _VimMode__l(cnc_widget *w);
void _VimMode__h(cnc_widget *w);
void _VimMode__x(cnc_widget *w);
void _VimMode__0(cnc_widget *w);
void _VimMode__$(cnc_widget *w);

typedef enum
{
  MODE_CMD, // command mode
  MODE_INS  // insert mode

} cnc_terminal_mode;

typedef struct
{
  size_t min_width;
  size_t min_height;

  size_t scr_rows;
  size_t scr_cols;
  struct termios orig_term;
  bool in_raw_mode;
  cnc_terminal_mode mode;
  cnc_buffer *screen_buffer;
  size_t widgets_count;
  cnc_widget **widgets;
  cnc_widget *focused_widget;
  size_t cursor_row;
  size_t cursor_col;

} cnc_terminal;

// terminal functions declaration
// screen buffer index calculator
size_t _index_at_cr(cnc_terminal *t, size_t c, size_t r);

cnc_terminal *cnc_terminal_init(size_t min_width, size_t min_height);
bool cnc_terminal_get_size(cnc_terminal *t);
void cnc_terminal_set_mode(cnc_terminal *t, cnc_terminal_mode mode);
cnc_widget *cnc_terminal_add_widget(cnc_terminal *t, cnc_widget_type type);
bool cnc_terminal_setup_widgets(cnc_terminal *t);
void cnc_terminal_screenbuffer_reset_char(cnc_terminal *t, char c);
void cnc_terminal_screenbuffer_reset(cnc_terminal *t);
void cnc_terminal_focus_widget(cnc_terminal *t, cnc_widget *w);
cnc_widget *cnc_terminal_focused_widget(cnc_terminal *t);
void cnc_terminal_focus_next(cnc_terminal *t);
void cnc_terminal_redraw(cnc_terminal *t);
void cnc_terminal_update_screen_buffer(cnc_terminal *t);
void cnc_terminal_update_and_redraw(cnc_terminal *t);
bool cnc_terminal_row_fg_is_set(cnc_terminal *t, size_t row);
void cnc_terminal_set_row_fg(cnc_terminal *t, size_t row, const char *color);
void cnc_terminal_set_row_bg(cnc_terminal *t, size_t row, const char *color);
int cnc_terminal_getch(cnc_terminal *t);
int _cnc_terminal_get_user_input(cnc_terminal *t);
int cnc_terminal_get_user_input(cnc_terminal *t);
void cnc_terminal_destroy(cnc_terminal *t);

#endif