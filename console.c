/**
 * @file console.c
 * @brief Console handling implementation
 * @version 1.2
 * @date 2024-11-13
 */

#include "console.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma region includes
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma endregion includes

#pragma region typedef
#pragma endregion typedef

#pragma region Private Function Prototypes

static void handleBackspace(void);
static void handleEnter(void);
static void handleArrowKey(void);
static void handleArrow(unsigned int *historyIndex, int direction);
static void handlePrintableChar(unsigned char c);
static unsigned int flushCommandBuffer(unsigned int cursorPos, unsigned char *cmdBuf, unsigned char *cmdSrc, unsigned int cmdLen);
static unsigned int increaseCommandIndex(unsigned int *cmdIdx);
static void freeParsedArguments(char **argv);
static void processCommand(unsigned char *cmd, unsigned int repeating);
static void stripLeadingWhiteSpace(unsigned char *cmd);
static void stripTrailingWhiteSpace(unsigned char *cmd);
static int parseToArgv(char *cmd, char ***argv);
static void executeCommand(int argc, char **argv);
static void helpCommand(int argc, char **argv);

#pragma endregion Private Function Prototypes

#pragma region defines
#pragma endregion defines

#pragma region variables

static command_t *commandList = NULL;
static unsigned char consoleInputBuffer[CONSOLE_BUFFER_SIZE];
static unsigned int inputPosition = 0;
static unsigned char commandHistory[CONSOLE_HISTORY_LENGTH][CONSOLE_BUFFER_SIZE];
static unsigned int historyPosition[CONSOLE_HISTORY_LENGTH];
static unsigned int historyInsert;
static unsigned int historyOutput;
static unsigned int historyInsertWrap;
static unsigned int historyOutputWrap;
static unsigned int upArrowCount;
static const console_io_t *consoleIO;

#pragma endregion variables

#pragma region External Functions

/**
 * @brief Initializes the console with I/O handlers and command list
 *
 * This function initializes the console by setting up the command list and I/O handlers.
 * It first adds a built-in "help" command, then adds all user-defined commands from the
 * provided command array. The commands are stored in a linked list structure.
 *
 * @param io Pointer to console_io_t structure containing I/O function handlers
 * @param commands Pointer to array of command_t structures defining available commands.
 *                 The array must be NULL-terminated (last command's name must be NULL)
 *
 * @note This function dynamically allocates memory for each command. If allocation fails,
 *       the function will return early and print a debug message if debug_print is available.
 *
 * @note After initialization, if debug printing is enabled, the function will print
 *       a list of all available commands.
 */
void consoleInit(const console_io_t *io, const command_t *commands) {
    const command_t *cmdPtr = commands;

    // Add help command first
    static command_t helpCmd = {"help", helpCommand, NULL};
    helpCmd.next             = NULL;
    commandList              = &helpCmd;
    command_t *lastCmd       = commandList;

    // Add remaining commands in original order
    while (cmdPtr && cmdPtr->command != NULL) {
        command_t *cmdCopy = (command_t *)malloc(sizeof(command_t));
        if (cmdCopy == NULL) {
            if (consoleIO && consoleIO->debug_print) {
                consoleIO->debug_print("Failed to allocate memory for command\r\n");
            }
            return;
        }
        memcpy(cmdCopy, cmdPtr, sizeof(command_t));
        cmdCopy->next = NULL;

        // Append to end of list
        lastCmd->next = cmdCopy;
        lastCmd       = cmdCopy;

        cmdPtr++;
    }

    consoleIO = io;

    // Debug print available commands
    if (io && io->debug_print) {
        io->debug_print("Available commands:\r\n");
        command_t *curr = commandList;
        while (curr) {
            io->debug_print("  %s\r\n", curr->command);
            curr = curr->next;
        }
        io->debug_print("\r\n");
    }
}

/**
 * @brief Console input handler function
 *
 * Processes individual characters received from the console input.
 * Handles special characters including:
 * - Backspace ('\b' or DEL)
 * - Enter ('\r')
 * - Arrow keys (starting with '[')
 * - Printable characters
 *
 * The function reads a single character from consoleIO interface
 * and routes it to appropriate handler functions based on the
 * character received.
 */
void consoleHandler(void) {
    unsigned char c;
    c = consoleIO->getchar();
    switch (c) {
        case '\b':
        case '\x7f':  // backspace
            handleBackspace();
            break;
        case '\r':  // enter
            handleEnter();
            break;
        case '[':  // arrow key
            handleArrowKey();
            break;
        default:
            handlePrintableChar(c);
            break;
    }
}

#pragma endregion External Functions

#pragma region Private Functions

static void handleBackspace(void) {
    if (inputPosition > 0) {
        consoleIO->print("\b \b");
        inputPosition--;
    }
    consoleInputBuffer[inputPosition] = '\0';
}

static void handleEnter(void) {
    consoleIO->print("\r\n");
    if (inputPosition) {
        if (strcmp((const char *)consoleInputBuffer, (const char *)commandHistory[historyInsert])) {
            if (increaseCommandIndex(&historyInsert) == 1) {
                historyInsertWrap = 1;
            }
            memcpy(commandHistory[historyInsert], consoleInputBuffer, CONSOLE_BUFFER_SIZE);
            historyPosition[historyInsert] = inputPosition;
        }
        historyOutput     = historyInsert;
        historyOutputWrap = 0;
        upArrowCount      = 0;
        processCommand(consoleInputBuffer, 0);
        inputPosition = 0;
        memset(consoleInputBuffer, 0, CONSOLE_BUFFER_SIZE);
        consoleIO->print("\r\n");
    }
    consoleIO->print("> ");
}

static void handleArrowKey(void) {
    unsigned char c = consoleIO->getchar();
    switch (c) {
        case 'A':  // up arrow
            handleArrow(&historyOutput, -1);
            break;
        case 'B':  // down arrow
            handleArrow(&historyOutput, 1);
            break;
        case 'C':  // right arrow
            break;
        case 'D':  // left arrow
            break;
        default:
            break;
    }
}

static void handleArrow(unsigned int *historyIndex, int direction) {
    if (direction == -1) {  // up arrow
        if (historyOutputWrap == 1 && *historyIndex == historyInsert) {
            return;
        }
        if (historyInsertWrap == 0 && *historyIndex == 0) {
            return;
        }
        upArrowCount++;
    } else if (direction == 1) {  // down arrow
        if (upArrowCount <= 1) {
            return;
        }
        upArrowCount--;
    }

    flushCommandBuffer(inputPosition, consoleInputBuffer, commandHistory[*historyIndex], historyPosition[*historyIndex]);
    inputPosition                         = historyPosition[*historyIndex];
    consoleInputBuffer[inputPosition + 1] = '\0';
    consoleIO->print("%s", consoleInputBuffer);

    if (direction == -1) {
        if (historyInsertWrap == 1) {
            if (*historyIndex == 0) {
                *historyIndex     = CONSOLE_HISTORY_LENGTH - 1;
                historyOutputWrap = 1;
            } else {
                (*historyIndex)--;
            }
        } else {
            if (*historyIndex != 0) {
                (*historyIndex)--;
            }
        }
    } else if (direction == 1) {
        increaseCommandIndex(historyIndex);
    }
}

static void handlePrintableChar(unsigned char c) {
    if (inputPosition < (CONSOLE_BUFFER_SIZE - 1) && (c >= ' ' && c <= 'z')) {
        consoleInputBuffer[inputPosition++] = c;
        consoleInputBuffer[inputPosition]   = '\0';
        consoleIO->print("%s", consoleInputBuffer + inputPosition - 1);
    }
    if (c == '\x7e') {
        consoleInputBuffer[inputPosition++] = c;
        consoleInputBuffer[inputPosition]   = '\0';
        consoleIO->print("%s", consoleInputBuffer + inputPosition - 1);
    }
}

static unsigned int flushCommandBuffer(unsigned int cursorPos, unsigned char *cmdBuf, unsigned char *cmdSrc, unsigned int cmdLen) {
    if (cursorPos > 0) {
        for (; cursorPos > 0; cursorPos--) {
            consoleIO->print("\b \b");
            consoleInputBuffer[cursorPos] = '\0';
        }
    }
    memcpy(cmdBuf, cmdSrc, cmdLen);
    return 0;
}

static unsigned int increaseCommandIndex(unsigned int *cmdIdx) {
    unsigned int localIdx;
    unsigned int ret = 0;

    localIdx = *cmdIdx;
    localIdx++;
    if (localIdx == CONSOLE_HISTORY_LENGTH) {
        localIdx = 0;
        ret      = 1;
    }
    *cmdIdx = localIdx;
    return ret;
}

static void freeParsedArguments(char **argv) {
    if (argv) {
        free(argv);
    }
}

static void processCommand(unsigned char *cmd, unsigned int repeating) {
    (void)repeating;

    stripLeadingWhiteSpace(cmd);
    stripTrailingWhiteSpace(cmd);

    int argc    = 0;
    char **argv = NULL;

    argc = parseToArgv((char *)cmd, &argv);

    unsigned int idx = 0;
    while (cmd[idx] != '\0') {
        if (cmd[idx] == ' ' || cmd[idx] == '\t' || cmd[idx] == '\r' || cmd[idx] == '\n') {
            cmd[idx] = '\0';
            break;
        }
        idx++;
    }

    if (argc > 0) {
        executeCommand(argc, argv);
    } else {
        if (consoleIO && consoleIO->debug_print) {
            consoleIO->debug_print("command `%s' not found, try `all help'\r\n", (argc == 0) ? "" : argv[0]);
        }
    }

    if (argv != NULL) {
        freeParsedArguments(argv);
        argv = NULL;
    }
}

static void stripLeadingWhiteSpace(unsigned char *cmd) {
    unsigned int idx  = 0;
    unsigned int copy = 0;

    while (cmd[idx] != '\0') {
        if (!isspace(cmd[idx])) {
            break;
        }
        idx++;
    }

    if (idx > 0) {
        while (cmd[idx] != '\0') {
            cmd[copy++] = cmd[idx++];
        }
        cmd[copy] = '\0';
    }
}

static void stripTrailingWhiteSpace(unsigned char *cmd) {
    unsigned int idx = strlen((const char *)cmd);
    while (idx > 0) {
        idx--;
        if (isspace(cmd[idx])) {
            cmd[idx] = '\0';
        } else {
            break;
        }
    }
}

static int parseToArgv(char *cmd, char ***argv) {
    int argc     = 0;
    int max_args = 10;
    *argv        = (char **)malloc(max_args * sizeof(char *));
    if (*argv == NULL) {
        if (consoleIO && consoleIO->debug_print) {
            consoleIO->debug_print("Failed to allocate memory for argv\r\n");
        }
        return -1;
    }

    char *token = strtok(cmd, " \t\r\n");
    while (token != NULL) {
        if (argc >= max_args) {
            max_args *= 2;
            *argv = (char **)realloc(*argv, max_args * sizeof(char *));
            if (*argv == NULL) {
                if (consoleIO && consoleIO->debug_print) {
                    consoleIO->debug_print("Failed to reallocate memory for argv\r\n");
                }
                return -1;
            }
        }

        (*argv)[argc++] = token;
        if (consoleIO && consoleIO->debug_print) {
            consoleIO->debug_print("Parsed argument %d: %s\r\n", argc - 1, token);
        }
        token = strtok(NULL, " \t\r\n");
    }

    if (consoleIO && consoleIO->debug_print) {
        consoleIO->debug_print("Total arguments parsed: %d\r\n", argc);
    }
    return argc;
}

static void executeCommand(int argc, char **argv) {
    command_t *currentCommand = commandList;
    int found                 = 0;

    while (currentCommand) {
        if (strcmp(currentCommand->command, argv[0]) == 0) {
            (currentCommand->function)(argc, argv);
            found = 1;
            break;
        }
        currentCommand = currentCommand->next;
    }

    if (!found) {
        if (consoleIO && consoleIO->debug_print) {
            consoleIO->debug_print("command `%s' not found, try `all help'\r\n", (argc == 0) ? "" : argv[0]);
        }
    }
}

/**
 * @brief Default help command to list all registered commands.
 *
 * This function displays a list of all commands that have been registered in the command list.
 * It iterates through the linked list of commands and prints each command name.
 * The output is formatted with commands indented by 2 spaces and each on a new line.
 *
 * @param argc Number of arguments (unused but required by command handler signature)
 * @param argv Array of argument strings (unused but required by command handler signature)
 *
 * @note Both argc and argv parameters are explicitly void-cast as they are not used
 *       in the function but are required by the command handler interface.
 *
 * @see commandList
 * @see command_t
 * @see consoleIO
 */
static void helpCommand(int argc, char **argv) {
    (void)argc;
    (void)argv;
    consoleIO->print("Available commands:\r\n");
    command_t *currentCommand = commandList;
    while (currentCommand) {
        consoleIO->print("  %s\r\n", currentCommand->command);
        currentCommand = currentCommand->next;
    }
}

#pragma endregion Private Functions

#ifdef __cplusplus
}
#endif
