/**
 * @file vt52emu.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief VT52 terminal emulator interface
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header defines the interface for VT52 terminal emulation,
 * providing escape sequence processing and terminal control functions.
 */

#ifndef _VT52EMU_H
#define _VT52EMU_H

#include "../../../../include/tdefs.h"

// VT52 emulator constants
#define VT52_MAX_PARAMS     8   ///< Maximum number of parameters in escape sequences

// VT52 emulator modes
typedef enum {
    VT52_MODE_NORMAL = 0,       ///< Normal character processing
    VT52_MODE_ESCAPE,           ///< Processing escape sequence
    VT52_MODE_CURSOR_POS,       ///< Processing cursor position sequence
    VT52_MODE_ANSI              ///< Processing ANSI escape sequence (extension)
} vt52_mode_t;

/**
 * @brief Initialize VT52 terminal emulator
 * 
 * Initializes the VT52 emulator state and underlying VGA screen.
 * Should be called once during system initialization.
 */
void vt52_init(void);

/**
 * @brief Process a character through VT52 emulator
 * 
 * Processes a single character, handling escape sequences and
 * terminal control functions according to VT52 specification.
 * 
 * @param c Character to process
 */
void vt52_process_char(char c);

/**
 * @brief Write string through VT52 emulator
 * 
 * Processes a null-terminated string character by character,
 * handling any embedded escape sequences.
 * 
 * @param str Null-terminated string to process
 */
void vt52_write_string(const char *str);

/**
 * @brief Clear screen via VT52 command
 * 
 * Sends VT52 escape sequence to clear screen and home cursor.
 */
void vt52_clear_screen(void);

/**
 * @brief Set cursor position via VT52 command
 * 
 * Sends VT52 direct cursor addressing sequence.
 * 
 * @param x Column position (0-based)
 * @param y Row position (0-based)
 */
void vt52_set_cursor(int x, int y);

/**
 * @brief Reset VT52 terminal
 * 
 * Resets the terminal to initial state with default colors
 * and clears the screen.
 */
void vt52_reset(void);

// Common VT52 escape sequences (for reference)
#define VT52_ESC            "\033"      ///< Escape character
#define VT52_CURSOR_UP      "\033A"     ///< Move cursor up
#define VT52_CURSOR_DOWN    "\033B"     ///< Move cursor down
#define VT52_CURSOR_RIGHT   "\033C"     ///< Move cursor right
#define VT52_CURSOR_LEFT    "\033D"     ///< Move cursor left
#define VT52_CURSOR_HOME    "\033H"     ///< Move cursor to home position
#define VT52_REVERSE_LF     "\033I"     ///< Reverse line feed
#define VT52_CLEAR_EOS      "\033J"     ///< Clear to end of screen
#define VT52_CLEAR_EOL      "\033K"     ///< Clear to end of line
#define VT52_INSERT_LINE    "\033L"     ///< Insert line
#define VT52_DELETE_LINE    "\033M"     ///< Delete line
#define VT52_IDENTIFY       "\033Z"     ///< Identify terminal

// ANSI extensions (for enhanced functionality)
#define ANSI_CLEAR_SCREEN   "\033[2J"   ///< Clear entire screen
#define ANSI_CURSOR_HOME    "\033[H"    ///< Move cursor to home
#define ANSI_RESET_COLOR    "\033[0m"   ///< Reset to default colors
#define ANSI_SAVE_CURSOR    "\0337"     ///< Save cursor position
#define ANSI_RESTORE_CURSOR "\0338"     ///< Restore cursor position

#endif /* _VT52EMU_H */
