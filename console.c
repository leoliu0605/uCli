/**
 * @file console.c
 * @brief Console handling implementation
 * @version 1.0
 * @date 2024-10-11
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
/**
 * @brief Handle backspace key press.
 */
static void handleBackspace(void);

/**
 * @brief Handle enter key press.
 */
static void handleEnter(void);

/**
 * @brief Handle arrow key press.
 */
static void handleArrowKey(void);

/**
 * @brief Handle specific arrow key direction.
 *
 * @param historyIndex Pointer to the history index.
 * @param direction Direction of the arrow key (-1 for up, 1 for down).
 */
static void handleArrow(unsigned int *historyIndex, int direction);

/**
 * @brief Handle printable character input.
 *
 * @param c The input character.
 */
static void handlePrintableChar(unsigned char c);

/**
 * @brief Flush the command buffer.
 *
 * @param cursorPos Current cursor position.
 * @param cmdBuf Command buffer to flush into.
 * @param cmdSrc Source command buffer.
 * @param cmdLen Length of the command.
 * @return unsigned int
 */
static unsigned int flushCommandBuffer(unsigned int cursorPos, unsigned char *cmdBuf, unsigned char *cmdSrc, unsigned int cmdLen);

/**
 * @brief Increase the command index.
 *
 * @param cmdIdx Pointer to the command index.
 * @return unsigned int
 */
static unsigned int increaseCommandIndex(unsigned int *cmdIdx);

/**
 * @brief Duplicate a string.
 *
 * @param str The input string.
 * @return char*
 */
static char *duplicateString(const char *str);

/**
 * @brief Set arguments from a command string.
 *
 * @param args The command string.
 * @param argv Array to store the arguments.
 * @return int
 */
static int setArguments(char *args, char **argv);

/**
 * @brief Parse arguments from a command string.
 *
 * @param args The command string.
 * @param argc Pointer to store the argument count.
 * @return char**
 */
static char **parseArguments(char *args, int *argc);

/**
 * @brief Free parsed arguments.
 *
 * @param argv The argument array.
 */
static void freeParsedArguments(char **argv);

/**
 * @brief Process a command.
 *
 * @param cmd The command string.
 * @param repeating Whether the command is repeating.
 */
static void processCommand(unsigned char *cmd, unsigned int repeating);

/**
 * @brief Strip leading white space from a command string.
 *
 * @param cmd The command string.
 */
static void stripLeadingWhiteSpace(unsigned char *cmd);

/**
 * @brief Strip trailing white space from a command string.
 *
 * @param cmd The command string.
 */
static void stripTrailingWhiteSpace(unsigned char *cmd);

/**
 * @brief Execute a command.
 *
 * @param argv The argument array.
 */
static void executeCommand(char **argv);

/**
 * @brief Default help command to list all registered commands.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void helpCommand(int argc, char **argv);

#pragma endregion Private Function Prototypes

#pragma region defines
#pragma endregion defines

#pragma region variables

/**
 * @brief List of registered commands.
 */
static command_t *commandList = NULL;

/**
 * @brief Buffer for console input.
 */
static unsigned char consoleInputBuffer[CONSOLE_BUFFER_SIZE];

/**
 * @brief Current position in the input buffer.
 */
static unsigned int inputPosition = 0;

/**
 * @brief History of entered commands.
 */
static unsigned char commandHistory[CONSOLE_HISTORY_LENGTH][CONSOLE_BUFFER_SIZE];

/**
 * @brief Positions of commands in the history.
 */
static unsigned int historyPosition[CONSOLE_HISTORY_LENGTH];

/**
 * @brief Index for inserting into the history.
 */
static unsigned int historyInsert;

/**
 * @brief Index for outputting from the history.
 */
static unsigned int historyOutput;

/**
 * @brief Wrap flag for history insertion.
 */
static unsigned int historyInsertWrap;

/**
 * @brief Wrap flag for history output.
 */
static unsigned int historyOutputWrap;

/**
 * @brief Counter for up arrow presses.
 */
static unsigned int upArrowCount;

/**
 * @brief Console I/O functions.
 */
static console_io_t *consoleIO;

#pragma endregion variables

#pragma region External Functions

/**
 * @brief Initialize the console with a list of commands and I/O functions.
 *
 * @param commands The list of commands.
 * @param io The console I/O functions.
 */
void consoleInit(command_t *commands, console_io_t *io) {
    command_t *prev = NULL;
    command_t *curr = NULL;

    // Add help command
    static command_t helpCmd = {"help", helpCommand, NULL};
    helpCmd.next             = commands;
    commands                 = &helpCmd;

    commands->next = NULL;
    if (commandList == NULL) {
        commandList = commands;
    } else {
        curr = commandList;
        while (curr != NULL) {
            if (strcmp(commands->command, curr->command) <= 0) {
                commands->next = curr;
                if (prev) {
                    prev->next = commands;
                } else {
                    commandList = commands;
                }
                break;
            }
            prev = curr;
            curr = curr->next;
        }
        if (curr == NULL) {
            prev->next = commands;
        }
    }

    consoleIO = io;
}

/**
 * @brief Handle console input.
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
        if (strcmp(consoleInputBuffer, commandHistory[historyInsert])) {
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

static char *duplicateString(const char *str) {
    size_t size;
    char *copy;

    size = strlen(str) + 1;
    copy = (char *)malloc(size);
    if (copy == NULL) {
        // Error handling
        consoleIO->debug_print("Memory allocation failed\n");
        return NULL;
    }
    memcpy(copy, str, size);
    return copy;
}

static int setArguments(char *args, char **argv) {
    int count = 0;

    while (isspace(*args)) ++args;
    while (*args) {
        if (argv) argv[count] = args;
        while (*args && !isspace(*args)) ++args;
        if (argv && *args) *args++ = '\0';
        while (isspace(*args)) ++args;
        count++;
    }
    return count;
}

static char **parseArguments(char *args, int *argc) {
    char **argv  = NULL;
    int argCount = 0;

    if (args && *args) {
        args = (char *)duplicateString(args);
        if (args) {
            argCount = setArguments(args, NULL);
            if (argCount) {
                argv = (char **)malloc((argCount + 1) * sizeof(char *));
                if (argv) {
                    *argv++  = args;
                    argCount = setArguments(args, argv);
                }
            }
        }
    }
    if (args && !argv) free(args);

    *argc = argCount;
    return argv;
}

static void freeParsedArguments(char **argv) {
    if (argv) {
        free(argv[-1]);
        free(argv - 1);
    }
}

static void processCommand(unsigned char *cmd, unsigned int repeating) {
    stripLeadingWhiteSpace(cmd);
    stripTrailingWhiteSpace(cmd);
    int argc = parseToArgv((char *)cmd, &argv);
    cmd[idx] = '\0';

    if (argc > 0) {
        executeCommand(argv);
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
    unsigned int idx = strlen(cmd);
    while (idx > 0) {
        idx--;
        if (isspace(cmd[idx])) {
            cmd[idx] = '\0';
        } else {
            break;
        }
    }
}

static void executeCommand(char **argv) {
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
        consoleIO->debug_print("command `%s' not found, try `all help'\r\n", (argc == 0) ? "" : argv[0]);
    }
}

/**
 * @brief Default help command to list all registered commands.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 */
static void helpCommand(int argc, char **argv) {
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
