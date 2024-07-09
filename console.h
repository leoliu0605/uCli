/**
 * @file console.h
 * @brief Console handling header
 * @version 1.0
 * @date 2024-10-11
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
 * @brief Structure representing a command.
 */
typedef struct command_t {
    const char *command;                     /**< Command string */
    void (*function)(int argc, char **argv); /**< Function to execute the command */
    struct command_t *next;                  /**< Pointer to the next command in the list */
} command_t;

/**
 * @brief Structure representing console I/O functions.
 */
typedef struct {
    void (*debug_print)(const char *format, ...);
    void (*print)(const char *format, ...);
    int (*getchar)(void);
} console_io_t;

#pragma endregion typedef

#pragma region defines

#define CONSOLE_BUFFER_SIZE    128
#define CONSOLE_HISTORY_LENGTH 4

#pragma endregion defines

#pragma region Exported Functions

/**
 * @brief Initialize the console with a list of commands and I/O functions.
 *
 * @param commands The list of commands.
 * @param io The console I/O functions.
 */
void consoleInit(command_t *commands, console_io_t *io);

/**
 * @brief Handle console input.
 */
void consoleHandler(void);

#pragma endregion Exported Functions

#ifdef __cplusplus
}
#endif

#endif  // CONSOLE_H
