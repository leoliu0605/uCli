# Console Command Handler

This project provides a console command handler for embedded systems, designed to be used with UART for receiving and sending commands. The handler is abstracted to allow for easy integration with different platforms and MCUs.

## Features

- Command registration and execution
- Command history management
- Abstracted I/O functions for cross-platform compatibility
- Lightweight and easy to integrate

## Getting Started

### Prerequisites

- A C compiler
- An embedded system with UART support

### Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/your-repo/console-command-handler.git
    ```

2. Include the `console.h` and `console.c` files in your project.

### Usage

#### Define Commands

Define your commands in a `command_t` array:

```c
#include "console.h"

void helloCommand(int argc, char **argv) {
    printf("Hello, World!\n");
}

command_t commands[] = {
    {"hello", helloCommand, NULL},
    {NULL, NULL, NULL}  // End of commands
};
```

#### Implement Platform-Specific I/O Functions

Implement the I/O functions for your platform. For example, using standard I/O:

```c
#include <stdio.h>
#include <stdarg.h>
#include "console.h"

void std_debug_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void std_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int std_getchar(void) {
    return getchar();
}

console_io_t std_io = {
    .debug_print = std_debug_print,
    .print = std_print,
    .getchar = std_getchar
};
```

#### Initialize and Handle Console

Initialize the console with the commands and I/O functions, and handle the console input in your main loop:

```c
#include "console.h"

int main(void) {
    consoleInit(commands, &std_io);

    while (1) {
        consoleHandler();
        // Other main loop logic
    }

    return 0;
}
```

### Example

Here is a complete example:

```c
/**
 * @file main.c
 * @brief Example usage of the console command handler
 * @version 0.1
 * @date 2024-09-10
 */

#include <stdio.h>
#include <stdarg.h>
#include "console.h"

void helloCommand(int argc, char **argv) {
    printf("Hello, World!\n");
}

command_t commands[] = {
    {"hello", helloCommand, NULL},
    {NULL, NULL, NULL}  // End of commands
};

void std_debug_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void std_print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int std_getchar(void) {
    return getchar();
}

console_io_t std_io = {
    .debug_print = std_debug_print,
    .print = std_print,
    .getchar = std_getchar
};

int main(void) {
    consoleInit(commands, &std_io);

    while (1) {
        consoleHandler();
        // Other main loop logic
    }

    return 0;
}
```

### License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Acknowledgments

- Inspired by various embedded system console handlers.
- Created by Leo in 2024.
