#include "screen.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>

#define MAX(a, b) (((a) < (b)) ? (b) : (a))

static int s_initialized = 0;

static int int_to_str(int val, char* buf, size_t buf_size)
{
    if (buf_size == 0) return 0;
    if (val == 0) {
        if (buf_size < 2) return 0;
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }
    char temp[32];
    int i = 0;
    unsigned int uval = (val < 0) ? (unsigned int)(-val) : (unsigned int)val;
    while (uval > 0 && i < 30) {
        temp[i++] = (char)('0' + (uval % 10));
        uval /= 10;
    }
    if (val < 0 && i < 30) {
        temp[i++] = '-';
    }
    size_t len = (size_t)i;
    if (len + 1 > buf_size) return 0;
    for (size_t j = 0; j < len; j++) {
        buf[j] = temp[len - 1 - j];
    }
    buf[len] = '\0';
    return (int)len;
}

static void S_reset_terminal();

static void S_handle_screen_change(int unused);

static void S_handle_signal(int sig);

#ifdef __cplusplus

void Screen::handle_screen_change(int)
{
    S_handle_screen_change(0);
}

void Screen::handle_signal(int sig)
{
    S_handle_signal(sig);
}

void Screen::reset_terminal()
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
    if (!s_initialized) {
        return;
    }
    s_initialized = 0;

    char buf[64];
    char row_str[32];
    int row_len = int_to_str(m_total_rows, row_str, sizeof(row_str));

    size_t pos = 0;
    const char prefix[] = "\033[r\033[";
    for (size_t i = 0; prefix[i]; i++) buf[pos++] = prefix[i];
    for (int i = 0; i < row_len; i++) buf[pos++] = row_str[i];
    const char suffix[] = ";1H\n";
    for (size_t i = 0; suffix[i]; i++) buf[pos++] = suffix[i];
    buf[pos] = '\0';

    write(STDOUT_FILENO, buf, pos);
}

static void S_handle_screen_change(int unused)
{
    (void)unused;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        m_total_rows = w.ws_row;
        m_cols       = w.ws_col;
        int scroll_bottom = MAX(1, m_total_rows - m_persistent_rows);

        char buf[64];
        char bot_str[32];
        int bot_len = int_to_str(scroll_bottom, bot_str, sizeof(bot_str));

        size_t pos = 0;
        const char prefix[] = "\0337\033[1;";
        for (size_t i = 0; prefix[i]; i++) buf[pos++] = prefix[i];
        for (int i = 0; i < bot_len; i++) buf[pos++] = bot_str[i];
        const char suffix[] = "r\0338";
        for (size_t i = 0; suffix[i]; i++) buf[pos++] = suffix[i];
        buf[pos] = '\0';

        write(STDOUT_FILENO, buf, pos);
    }
}

static void S_handle_signal(int sig)
{
    S_reset_terminal();
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(sig, &sa, NULL);
    raise(sig);
}

void S_clear_screen()
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

void S_reset_buffer()
{
    printf("\033[3J");
    fflush(stdout);
}

void S_teardown()
{
    fflush(stdout);
    S_reset_terminal();
}

void S_initialize(int npersistent)
{
    if (npersistent < 0) {
        npersistent = 0;
    }
    m_persistent_rows = npersistent;
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
        m_total_rows = w.ws_row;
        m_cols       = w.ws_col;
    }

    if (!s_initialized) {
        struct sigaction sa;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sa.sa_mask);

        sa.sa_handler = S_handle_signal;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        sa.sa_handler = S_handle_screen_change;
        sigaction(SIGWINCH, &sa, NULL);

        atexit(S_reset_terminal);
        s_initialized = 1;
    }

    int scroll_bottom = MAX(1, m_total_rows - m_persistent_rows);

    printf("\033[1;%dr\033[0J", scroll_bottom);
    fflush(stdout);
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
    return MAX(1, m_total_rows - m_persistent_rows);
}

/**
 * Relative row number. If there are 3 persistent rows then row 1, 2, and 3
 * are the possible values for the row
 */
void S_persistent_write(int row, char const* fmt, ...)
{
    if (row < 1 || row > m_persistent_rows) {
        return;
    }
    int target_row = S_total_rows() - (S_persistent_rows() - row);
    if (target_row < 1) {
        target_row = 1;
    }

    va_list list;
    va_start(list, fmt);
    char* buf = (char*) malloc(S_total_cols() + 1);
    if (buf == NULL)
    #ifdef __cplusplus
    {throw std::bad_alloc();}
    #else
    {fprintf(stderr, "Failed to allocate\n"); exit(1);}
    #endif
    // only write the number of chars that fit on a line
    vsnprintf(buf, S_total_cols() + 1, fmt, list);
    va_end(list);
    printf("\0337\033[%d;1H\033[0K%s\0338", target_row, buf);
    fflush(stdout);
    free(buf);
}

void S_scroll_write(char const* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char* buf = (char*) malloc(S_total_cols() + 1);
    if (buf == NULL)
    #ifdef __cplusplus
    {throw std::bad_alloc();}
    #else
    {fprintf(stderr, "Failed to allocate\n"); exit(1);}
    #endif

    // only write the number of chars that fit on a line
    vsnprintf(buf, S_total_cols() + 1, fmt, list);
    va_end(list);
    printf("%s\n", buf);
    fflush(stdout);
    free(buf);
}

#ifdef __cplusplus
}
#endif
