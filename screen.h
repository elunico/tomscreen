#ifndef SCREEN_HPP
#define SCREEN_HPP

#pragma once

#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int m_persistent_rows;
extern int m_total_rows;
extern int m_cols;

void S_initialize(int npersistent);

int S_total_cols();

int S_total_rows();

int S_persistent_rows();

int S_scrollable_rows();

/**
 * Relative row number. If there are 3 persistent rows then row 1, 2, and 3
 * are the possible values for the row
 */
void S_persistent_write(int row, char const* fmt, ...);

void S_scroll_write(char const* fmt, ...);

void S_teardown();

void S_clear_screen();

void S_reset_buffer();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <format>
#include <iostream>
#include <sstream>
#include <exception>
#include <memory>
#include <string>
#include <print>

class Screen {
    static void handle_screen_change(int);

    static void handle_signal(int sig);

    static void reset_terminal();

   public:
    Screen()                         = delete;
    Screen& operator=(Screen const&) = delete;

    static void clear_screen()
    {
        // std::cout << "\033[2J" << std::endl;
        S_clear_screen();
    }

    static void reset_buffer()
    {
        S_reset_buffer();
        // std::cout << "\033[3J" << std::endl;
    }

    static void teardown()
    {
        S_teardown();
    }

    static void inititalize(int npersistent)
    {
        S_initialize(npersistent);
    }

    static int total_cols()
    {
        // return m_cols;
        return S_total_cols();
    }

    static int persistent_rows()
    {
        return S_persistent_rows();
    }

    static int total_rows()
    {
        return S_total_rows();
    }

    static int scrollable_rows()
    {
        return S_scrollable_rows();
    }

    /**
     * Relative row number. If there are 3 persistent rows then row 1, 2, and 3
     * are the possible values for the row
     */
    template <typename... Args>
    static void persistent_write(int row, Args&&... args)
    {
        if (row < 1 || row > persistent_rows()) {
            return;
        }
        int target_row = total_rows() - (persistent_rows() - row);
        if (target_row < 1) {
            target_row = 1;
        }
        std::print("\0337\033[{};1H\033[0K", target_row);

        (std::print("{}", args), ...);
        std::print("\0338");
        std::cout << std::flush;
    }

    template <typename... Args>
    static void scroll_write(Args&&... args)
    {
        std::print("\033[0K");
        (std::print("{}", args), ...);
        std::cout << std::endl;
    }
};

#endif

#endif
