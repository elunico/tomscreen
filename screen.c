#include "screen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>

#define MAX(a, b) (((a) < (b)) ? (b) : (a))

static void S_reset_terminal();

static void S_handle_screen_change(int unused);

static void S_handle_signal(int sig);

#ifdef __cplusplus

static void Screen::handle_screen_change(int)
{
    S_handle_screen_change(0);
}

static void Screen::handle_signal(int sig)
{
    S_handle_signal(sig);
}

static void Screen::reset_terminal()
{
    S_reset_terminal();
}

extern "C" {
#endif
int m_persistent_rows = 0;
int m_total_rows      = 24;
int m_cols            = 80;

static void S_reset_terminal()
{
    char buf[20];
    int  count =
        snprintf(buf, 20, "%s%d%s", "\033[r\033[", m_total_rows, ";1H\n");
    write(STDOUT_FILENO, buf, count);
}

static void S_handle_screen_change(int unused)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        m_total_rows = w.ws_row;
        m_cols       = w.ws_col;
        char buf[15];
        int  scroll_bottom = MAX(1, m_total_rows - m_persistent_rows);
        snprintf(buf, 15, "%s%d%s", "\033[1;", scroll_bottom, "r");
        write(STDOUT_FILENO, buf, 15);
    }
}

static void S_handle_signal(int sig)
{
    S_reset_terminal();
    signal(sig, SIG_DFL);
    raise(sig);
}

void S_clear_screen()
{
    printf("%s\n", "\033[2J");
}

void S_reset_buffer()
{
    printf("%s\n", "\033[3J");
}

void S_teardown()
{
    S_reset_terminal();
}

void S_initialize(int npersistent)
{
    m_persistent_rows = npersistent;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        m_total_rows = w.ws_row;
        m_cols       = w.ws_col;
    }

    signal(SIGINT, S_handle_signal);
    signal(SIGTERM, S_handle_signal);
    signal(SIGWINCH, S_handle_screen_change);
    atexit(S_reset_terminal);

    int scroll_bottom = MAX(1, m_total_rows - m_persistent_rows);

    printf("%s%d%s\n", "\033[1;", scroll_bottom, "r\033[0J");
}

int S_total_cols()
{
    return m_cols;
}

int S_persistent_rows()
{
    return m_persistent_rows;
}

int S_total_rows()
{
    return m_total_rows;
}

int S_scrollable_rows()
{
    return m_total_rows - m_persistent_rows;
}

/**
 * Relative row number. If there are 3 persistent rows then row 1, 2, and 3
 * are the possible values for the row
 */
void S_persistent_write(int row, char const* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char* buf = (char*) malloc(S_total_cols());
    // only write the number of chars that fit on a line
    vsnprintf(buf, S_total_cols(), fmt, list);
    va_end(list);
    printf("\0337\033[%d;1H\033[0K%s\0338",
           (S_total_rows() - (S_persistent_rows() - row)), buf);
    fflush(stdout);
}

void S_scroll_write(char const* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char* buf = (char*) malloc(S_total_cols());
    // only write the number of chars that fit on a line
    vsnprintf(buf, S_total_cols(), fmt, list);
    va_end(list);
    printf("%s\n", buf);
    fflush(stdout);
}

#ifdef __cplusplus
}
#endif
