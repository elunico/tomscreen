# tomscreen

A lightweight C and C++ terminal UI library for creating persistent (pinned) bottom rows while maintaining an active scrolling output region above. Provides for apt-like output--that is the scrolling details above a persistent message or status bar.

## Overview

`tomscreen` enables command-line programs to log continuous scrolling output in the upper portion of the terminal while dedicating the bottom _N_ rows to persistent status bars, progress indicators, or interactive controls.

### Key Features

- **Split Terminal Regions**: Dedicates the top rows for regular scrolling log messages and the bottom rows for pinned, persistent information.
- **Dynamic Resizing**: Automatically listens for `SIGWINCH` and recalculates terminal dimensions on window resize.
- **Safe Signal & Exit Handling**: Installs signal handlers for `SIGINT` and `SIGTERM` as well as an `atexit` handler to ensure the terminal scrolling region and cursor position are properly restored upon exit.
- **Dual C and C++ Support**: Provides a clean C API with `printf`-style formatting and a modern C++ wrapper with variadic template formatting (`std::print`).
- **Zero Heavy Dependencies**: Implemented using standard ANSI escape sequences and POSIX system calls (`ioctl`, `signal`, `unistd.h`).

---

## Building the Project

`tomscreen` uses CMake to generate a static library (`libtomscreen.a`).

### Prerequisites

- A C17-compatible compiler (e.g., `clang` or `gcc`)
- CMake 3.13 or higher
- _(Optional, for C++ usage)_ A C++23-compatible compiler supporting `<print>` (such as GCC 14+ or Clang 17+)

### Build Steps

1. Clone the repository:

   ```bash
   git clone https://github.com/thomaspovinelli/tomscreen.git
   cd tomscreen
   ```

2. Configure and build:
   ```bash
   mkdir build
   cd build
   cmake .. -DNDEBUG=ON
   cmake --build .
   ```

To build in debug mode with AddressSanitizer and UndefinedBehaviorSanitizer enabled:

```bash
cmake .. -DNDEBUG=OFF
cmake --build .
```

The build will generate `libtomscreen.a` in the `build` directory.

---

## Adding to Your Project

### Adding to a CMake Project

#### Option A: Using `add_subdirectory` (Recommended if embedding source)

If you include `tomscreen` as a submodule or subdirectory in your project:

```cmake
cmake_minimum_required(VERSION 3.13)
project(MyProject)

# Include tomscreen
add_subdirectory(path/to/tomscreen)

# Define your executable
add_executable(my_app main.c)

# Include directory & link library
target_include_directories(my_app PRIVATE path/to/tomscreen)
target_link_libraries(my_app PRIVATE tomscreen)
```

#### Option B: Linking Pre-Built Static Library

If you built `libtomscreen.a` separately:

```cmake
cmake_minimum_required(VERSION 3.13)
project(MyProject)

add_executable(my_app main.c)

# Add header include path
target_include_directories(my_app PRIVATE /path/to/tomscreen)

# Link the static library
target_link_libraries(my_app PRIVATE /path/to/tomscreen/build/libtomscreen.a)
```

---

### Adding to a Regular Makefile

You can include `screen.h` and link `libtomscreen.a` directly in your Makefile:

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -I/path/to/tomscreen
LDFLAGS = -L/path/to/tomscreen/build -ltomscreen

# For C++:
# CXX = g++
# CXXFLAGS = -Wall -Wextra -std=c++23 -I/path/to/tomscreen

TARGET = my_app
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
```

---

## Example Usage in C

The C API provides `printf`-style formatted output functions. Persistent row indices are 1-based (from `1` to `N` persistent rows, where row 1 is the topmost persistent row).

```c
#include "screen.h"
#include <stdio.h>
#include <unistd.h>

int main(void) {
    // Initialize terminal with 2 persistent rows at the bottom
    S_initialize(2);

    // Initial persistent status display
    S_persistent_write(1, "Status: Initializing...");
    S_persistent_write(2, "Progress: [--------------------] 0%%");

    for (int i = 1; i <= 20; ++i) {
        // Output scrolling log messages to the upper region
        S_scroll_write("Processing item %d of 20...", i);

        // Update persistent rows at the bottom
        int percent = (i * 100) / 20;
        char bar[21];
        for (int b = 0; b < 20; ++b) {
            bar[b] = (b < i) ? '=' : '-';
        }
        bar[20] = '\0';

        S_persistent_write(1, "Status: Running batch task (active)");
        S_persistent_write(2, "Progress: [%s] %d%%", bar, percent);

        usleep(150000); // 150ms
    }

    S_scroll_write("Task completed successfully!");
    S_persistent_write(1, "Status: Finished!");
    S_persistent_write(2, "Press enter to exit...");

    getchar();

    // Clean up and restore terminal state
    S_teardown();
    return 0;
}
```

---

## Example Usage in C++

The C++ wrapper provides a static `Screen` class that leverages modern C++ variadic templates and `std::print`.

```cpp
#include "screen.h"
#include <chrono>
#include <string>
#include <thread>

int main() {
    // Initialize terminal with 2 persistent rows at the bottom
    Screen::inititalize(2);

    Screen::persistent_write(1, "Status: Initializing worker threads...");
    Screen::persistent_write(2, "Progress: 0%");

    for (int step = 1; step <= 10; ++step) {
        // Write scrolling log entries
        Screen::scroll_write("[LOG] Finished processing chunk #", step);

        // Update persistent bottom rows
        int percent = step * 10;
        Screen::persistent_write(1, "Status: In progress (step ", step, " of 10)");
        Screen::persistent_write(2, "Progress: ", percent, "% completed");

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    Screen::scroll_write("[LOG] All work completed!");
    Screen::persistent_write(1, "Status: Complete!");
    Screen::persistent_write(2, "Done.");

    // Clean up and restore terminal state
    Screen::teardown();
    return 0;
}
```
