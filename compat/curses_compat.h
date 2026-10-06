#ifndef BALANCE_CURSES_COMPAT_H
#define BALANCE_CURSES_COMPAT_H
#if defined(__has_include)
# if __has_include(<ncursesw/ncurses.h>)
#  include <ncursesw/ncurses.h>
#  define BALANCE_HAVE_NATIVE_CURSES 1
# elif __has_include(<ncurses.h>)
#  include <ncurses.h>
#  define BALANCE_HAVE_NATIVE_CURSES 1
# elif __has_include(<curses.h>)
#  include <curses.h>
#  define BALANCE_HAVE_NATIVE_CURSES 1
# endif
#endif
#ifndef BALANCE_HAVE_NATIVE_CURSES
#include <stdint.h>
typedef unsigned long chtype;
typedef unsigned long mmask_t;
typedef struct _bal_window WINDOW;
typedef struct { short id; int x,y,z; mmask_t bstate; } MEVENT;
#define ERR (-1)
#define OK 0
#define TRUE 1
#define FALSE 0
#define KEY_DOWN 258
#define KEY_UP 259
#define KEY_LEFT 260
#define KEY_RIGHT 261
#define KEY_HOME 262
#define KEY_BACKSPACE 263
#define KEY_DC 330
#define KEY_NPAGE 338
#define KEY_PPAGE 339
#define KEY_END 360
#define KEY_MOUSE 409
#define KEY_RESIZE 410
#define KEY_ENTER 343
extern WINDOW *stdscr;
#define BUTTON1_CLICKED 2UL
#define ALL_MOUSE_EVENTS 0xffffffffffffffffUL
#define A_BOLD 0UL
#define A_REVERSE 0UL
#define A_DIM 0UL
#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7
#define COLOR_PAIR(n) 0UL
#define KEY_F1 265
WINDOW *initscr(void);
int endwin(void);
int raw(void);
int noecho(void);
int echo(void);
int keypad(WINDOW*, int);
int start_color(void);
int has_colors(void);
int init_pair(short, short, short);
int use_default_colors(void);
mmask_t mousemask(mmask_t, mmask_t*);
int curs_set(int);
void timeout(int);
int wgetch(WINDOW*);
WINDOW *newwin(int,int,int,int);
int delwin(WINDOW*);
int werase(WINDOW*);
int wrefresh(WINDOW*);
int wmove(WINDOW*,int,int);
int mvwaddnstr(WINDOW*,int,int,const char*,int);
int mvwprintw(WINDOW*,int,int,const char*,...);
int waddnstr(WINDOW*,const char*,int);
int waddch(WINDOW*,const chtype);
int mvwaddch(WINDOW*,int,int,const chtype);
int scrollok(WINDOW*,int);
int wattron(WINDOW*,int);
int wattroff(WINDOW*,int);
int wbkgd(WINDOW*,chtype);
int wclear(WINDOW*);
int werase(WINDOW*);
int wclrtoeol(WINDOW*);
int getmouse(MEVENT*);
#endif
#endif
