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

### Using with STM32 and UART

To use this console handler with an STM32 MCU and UART, follow these steps:

1. **Initialize UART**: Initialize the UART peripheral in your STM32 project. This can be done using STM32CubeMX or manually in your code.

2. **Implement UART I/O Functions with Interrupts**: Implement the I/O functions to use the STM32 HAL UART functions with interrupts.

```c
#include "stm32f1xx_hal.h"
#include "console.h"

extern UART_HandleTypeDef huart1;  // Assuming UART1 is used

// Define a structure for UART receive buffer
typedef struct {
    uint8_t buffer[128];          // Buffer to store received data
    volatile uint16_t head;       // Head index for buffer
    volatile uint16_t tail;       // Tail index for buffer
} UART_RxBuffer_t;

#define RX_BUFFER_SIZE 128
static UART_RxBuffer_t rxBuffer = {{0}, 0, 0};  // Initialize the receive buffer

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        uint16_t nextHead = (rxBuffer.head + 1) % RX_BUFFER_SIZE;
        if (nextHead != rxBuffer.tail) {
            rxBuffer.buffer[rxBuffer.head] = huart->Instance->DR;
            rxBuffer.head = nextHead;
        }
        HAL_UART_Receive_IT(&huart1, rxBuffer.buffer + rxBuffer.head, 1);
    }
}

void stm32_debug_print(const char *format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void stm32_print(const char *format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

int stm32_getchar(void) {
    if (rxBuffer.head == rxBuffer.tail) {
        return -1;  // No data available
    } else {
        uint8_t ch = rxBuffer.buffer[rxBuffer.tail];
        rxBuffer.tail = (rxBuffer.tail + 1) % RX_BUFFER_SIZE;
        return ch;
    }
}

console_io_t stm32_io = {
    .debug_print = stm32_debug_print,
    .print = stm32_print,
    .getchar = stm32_getchar
};
```

3. **Initialize Console**: Initialize the console with the commands and STM32 UART I/O functions.

```c
#include "console.h"

void helloCommand(int argc, char **argv) {
    stm32_print("Hello, World!\n");
}

command_t commands[] = {
    {"hello", helloCommand, NULL},
    {NULL, NULL, NULL}  // End of commands
};

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_USART1_UART_Init();

    // Start UART reception in interrupt mode
    HAL_UART_Receive_IT(&huart1, rxBuffer.buffer + rxBuffer.head, 1);

    consoleInit(commands, &stm32_io);

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
