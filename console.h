/**
 * @file console.h
 * @brief Console handling header
 * @version 1.1
 * @date 2024-11-10
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#pragma region includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#pragma endregion includes

#pragma region typedef

/**
 * @brief Structure representing console I/O functions.
 *
 * @details This structure provides a standardized interface for console operations,
 * wrapping basic I/O functions like printf and getchar.
 *
 * Example usage:
 * @code
 * console_io_t console = {
 *     .debug_print = printf,
 *     .print = printf,
 *     .getchar = getchar
 * };
 *
 * console.print("Hello %s\n", "World");
 * int ch = console.getchar();
 * @endcode
 *
 * @field debug_print Function pointer for debug messages (printf-like format)
 * @field print Function pointer for normal output (printf-like format)
 * @field getchar Function pointer for character input
 */
typedef struct {
    void (*debug_print)(const char *format, ...);
    void (*print)(const char *format, ...);
    int (*getchar)(void);
} console_io_t;

/**
 * @brief Structure for command handling
 *
 * Example:
 * @code
 * command_t cmd = {
 *   .command = "help",
 *   .function = help_handler,
 *   .next = NULL
 * };
 * @endcode
 */
typedef struct command_t {
    const char *command;                     /**< Command string */
    void (*function)(int argc, char **argv); /**< Function to execute the command */
    struct command_t *next;                  /**< Pointer to the next command in the list */
} command_t;

#pragma endregion typedef

#pragma region defines

#define CONSOLE_BUFFER_SIZE    128
#define CONSOLE_HISTORY_LENGTH 4

#pragma endregion defines

#pragma region Exported Functions

void consoleInit(const console_io_t *io, const command_t *commands);
void consoleHandler(void);

#pragma endregion Exported Functions

#ifdef __cplusplus
}
#endif

#endif  // CONSOLE_H
